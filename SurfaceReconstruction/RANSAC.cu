#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "cusolverDn.h"
#include "curand.h"
#include "curand_kernel.h"
#include <exception>
#include <random>

//__device__ void printMat(float** mat, int n, int m)
//{
//	for (int i = 0; i < n; i++)
//	{
//		for (int j = 0; j < m; j++)
//		{
//			printf("%f ", mat[i][j]);
//		}
//		printf("\n");
//	}
//}

class CudaException : public std::exception
{
	virtual const char* what() const throw()
	{
		return "GPUassert fail";
	}
};

#define checkCudaCall(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char* file, int line, bool abort = true)
{
	if (code != cudaSuccess)
	{
		throw CudaException();
	}
}

__device__ float distance(float* model, float* p)
{
	return fabsf(model[0] * p[0] + model[1] * p[1] + model[2] * p[2] + model[3]);
}

__global__ void countInliers(float* pts, float* eigenvectors, int* count_inliers, float* plane_params, int size, float threshold)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;

	float A = eigenvectors[id * 16];
	float B = eigenvectors[id * 16 + 1];
	float C = eigenvectors[id * 16 + 2];
	float D = eigenvectors[id * 16 + 3];

	float norm = sqrtf(A * A + B * B + C * C);

	plane_params[id * 4] = A / norm;
	plane_params[id * 4 + 1] = B / norm;
	plane_params[id * 4 + 2] = C / norm;
	plane_params[id * 4 + 3] = D / norm;

	float model[4] = { plane_params[id * 4] , plane_params[id * 4 + 1] , plane_params[id * 4 + 2], plane_params[id * 4 + 3]};
	//float model[3] = { plane_params[id * 3] , plane_params[id * 3 + 1] , plane_params[id * 3 + 2]};

	int count = 0;
	for (int i = 0; i < size; i++)
	{
		float p[3] = { pts[i * 3], pts[i * 3 + 1], pts[i * 3 + 2]};
		if (distance(model, p) < threshold)
			count++;
	}

	count_inliers[id] = count;
}

__global__ void createSystems(float* pts, float* As, float** bs, int seed, curandState* rand_state, int size)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;

	curand_init(seed, id, 0, &rand_state[id]);
	int idx1, idx2, idx3;
	idx1 = curand_uniform(&(rand_state[id])) * size - 1;	//curand excludes 0.0 and includes 1.0
	idx2 = curand_uniform(&(rand_state[id])) * size - 1;
	idx3 = curand_uniform(&(rand_state[id])) * size - 1;

	float Aorig[3][4];
	Aorig[0][0] = pts[idx1 * 3];
	Aorig[0][1] = pts[idx1 * 3 + 1];
	Aorig[0][2] = pts[idx1 * 3 + 2];
	Aorig[0][3] = 1;
	Aorig[1][0] = pts[idx2 * 3];
	Aorig[1][1] = pts[idx2 * 3 + 1];
	Aorig[1][2] = pts[idx2 * 3 + 2];
	Aorig[1][3] = 1;
	Aorig[2][0] = pts[idx3 * 3];
	Aorig[2][1] = pts[idx3 * 3 + 1];
	Aorig[2][2] = pts[idx3 * 3 + 2];
	Aorig[2][3] = 1;
	//printf("%d, %d, %d\n", idx1, idx2, idx3);

	//printf("Aorig:\n");
	//for (int i = 0; i < 3; i++)
	//{
	//	for (int j = 0; j < 4; j++)
	//	{
	//		printf("%f ", Aorig[i][j]);
	//	}
	//	printf("\n");
	//}
	
	//printf("At:\n");
	float At[4][3];
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			At[i][j] = Aorig[j][i];
		}
	}

	//for (int i = 0; i < 4; i++)
	//{
	//	for (int j = 0; j < 3; j++)
	//	{
	//		printf("%f ", At[i][j]);
	//	}
	//	printf("\n");
	//}

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			As[id * 16 + j * 4 + i] = 0;

			for (int k = 0; k < 3; k++) {
				//Column major ordering
				As[id * 16 + j * 4 + i] += At[i][k] * Aorig[k][j];
			}
		}
	}

	//printf("A:\n");
	//for (int i = 0; i < 4; i++)
	//{
	//	for (int j = 0; j < 4; j++)
	//	{
	//		printf("%f ", As[id * 16 + i * 4 + j]);
	//	}
	//	printf("\n");
	//}

	//Put them in column major for cublas
	//x, y, z, 1
	//As[id * 16] = pts[idx1 * 3];
	//As[id][1] = pts[idx2 * 3];
	//As[id][2] = pts[idx3 * 3];
	//As[id][3] = pts[idx1 * 3 + 1];
	//As[id][4] = pts[idx2 * 3 + 1];
	//As[id][5] = pts[idx3 * 3 + 1];
	//As[id][6] = pts[idx1 * 3 + 2];
	//As[id][7] = pts[idx2* 3 + 2];
	//As[id][8] = pts[idx3 * 3 + 2];
	////As[id][9] = 1;
	////As[id][10] = 1;
	////As[id][11] = 1;

	////homogeneous system
	//bs[id][0] = -1;
	//bs[id][1] = -1;
	//bs[id][2] = -1;
	//bs[id][3] = 0;
}

cudaError_t runRansacGPU(const std::vector<float>& pts, std::vector<float>& plane_params, std::vector<int>& counts, const int& iters, const float& threshold, const int& threadNum)
{
	float* d_pts = 0;
	float* d_plane_params = 0;
	int* d_counts = 0;
	//float** d_As = 0;
	float* d_As = 0;
	float** d_bs = 0;

	int* h_counts = (int*)malloc(sizeof(int) * iters);
	float* h_plane_params = (float*)malloc(sizeof(float) * 4 * iters);
	float** h_As = (float**)malloc(iters * sizeof(float*));
	float** h_bs = (float**)malloc(iters * sizeof(float*));

	curandState* d_state;
	cudaError_t cudaStatus;

	cublasHandle_t cublasH = NULL;

	try {
		checkCudaCall(cudaSetDevice(0));

		checkCudaCall(cudaMalloc((void**)&d_pts, pts.size() * sizeof(float)));
		checkCudaCall(cudaMalloc((void**)&d_plane_params, iters * 4 * sizeof(float)));
		checkCudaCall(cudaMalloc((void**)&d_counts, iters * sizeof(float)));

		checkCudaCall(cudaMalloc(&d_state, iters * sizeof(curandState)));
		//checkCudaCall(cudaMalloc((void**)&d_As, iters * sizeof(float*)));
		checkCudaCall(cudaMalloc((void**)&d_As, iters * 16 * sizeof(float)));
		checkCudaCall(cudaMalloc((void**)&h_As[0], iters * 9 * sizeof(float)));
		checkCudaCall(cudaMalloc((void**)&d_bs, iters * sizeof(float*)));
		checkCudaCall(cudaMalloc((void**)&h_bs[0], iters * 3 * sizeof(float)));

		for (int i = 1; i < iters; i++)
		{
			h_As[i] = h_As[i - 1] + 9;
			h_bs[i] = h_bs[i - 1] + 3;
		}

		checkCudaCall(cudaMemcpy(d_pts, pts.data(), pts.size() * sizeof(float), cudaMemcpyHostToDevice));
		//checkCudaCall(cudaMemcpy(d_As, h_As, iters * sizeof(float*), cudaMemcpyHostToDevice));
		checkCudaCall(cudaMemcpy(d_bs, h_bs, iters * sizeof(float*), cudaMemcpyHostToDevice));

		srand(time(0));
		int seed = rand();

		//createSystems <<< 1, 1 >>> (d_pts, d_As, d_bs, seed, d_state, pts.size() / 3);
		createSystems <<< iters / threadNum, threadNum >>> (d_pts, d_As, d_bs, seed, d_state, pts.size() / 3);

		//////////////////
		cusolverDnHandle_t cusolverH = NULL;
		cudaStream_t stream = NULL;
		syevjInfo_t syevj_params = NULL;

		const int m = 4;
		const int lda = 4;
		const int batchSize = iters;

		//std::vector<float> A(lda * m * batchSize, 0); /* A = [A0 ; A1] */
		std::vector<float> V(lda * m * iters, 0); /* V = [V0 ; V1] */
		std::vector<float> W(m * iters, 0);       /* W = [W0 ; W1] */
		std::vector<int> info(batchSize, 0);           /* info = [info0 ; info1] */

		float* d_A = nullptr;    /* lda-by-m-by-batchSize */
		float* d_W = nullptr;    /* m-by-batchSize */
		int* d_info = nullptr;    /* batchSize */
		float* d_work = nullptr; /* device workspace for syevjBatched */
		int lwork = 0;            /* size of workspace */

		/* configuration of syevj  */
		const double tol = 1.e-7;
		const int max_sweeps = 15;
		const int sort_eig = 1;                                  /* don't sort eigenvalues */
		const cusolverEigMode_t jobz = CUSOLVER_EIG_MODE_VECTOR; /* compute eigenvectors */
		const cublasFillMode_t uplo = CUBLAS_FILL_MODE_LOWER;

		//double* A0 = A.data();
		//double* A1 = A.data() + lda * m;

		cusolverDnCreate(&cusolverH);
		cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking);
		cusolverDnSetStream(cusolverH, stream);
		cusolverDnCreateSyevjInfo(&syevj_params);
		cusolverDnXsyevjSetTolerance(syevj_params, tol);
		cusolverDnXsyevjSetMaxSweeps(syevj_params, max_sweeps);
		cusolverDnXsyevjSetSortEig(syevj_params, sort_eig);

		//cudaMalloc(reinterpret_cast<void**>(&d_A), sizeof(double) * A.size());
		cudaMalloc((void**)&d_W, sizeof(float) * W.size());
		cudaMalloc((void**)&d_info, sizeof(int) * info.size());
		//cudaMemcpyAsync(d_A, A.data(), sizeof(double) * A.size(), cudaMemcpyHostToDevice, stream));
		cusolverDnSsyevjBatched_bufferSize(cusolverH, jobz, uplo, m, d_As, lda, d_W, &lwork, syevj_params, batchSize);
		cudaMalloc(reinterpret_cast<void**>(&d_work), sizeof(double) * lwork);
		cusolverDnSsyevjBatched(cusolverH, jobz, uplo, m, d_As, lda, d_W, d_work, lwork, d_info, syevj_params, batchSize);
		cudaMemcpyAsync(V.data(), d_As, sizeof(float) * 16 * iters, cudaMemcpyDeviceToHost, stream);
		cudaMemcpyAsync(W.data(), d_W, sizeof(float) * 4 * iters, cudaMemcpyDeviceToHost, stream);
		cudaStreamSynchronize(stream);
		/////////////////

		//int* d_info = nullptr;
		//cublasCreate(&cublasH);
		//cublasSgelsBatched(cublasH, cublasOperation_t::CUBLAS_OP_N, 3, 3, 1, d_As, 1, d_bs, 1, d_info, NULL, iters);

		countInliers <<< iters / threadNum, threadNum >>> (d_pts, d_As, d_counts, d_plane_params, pts.size() / 3, threshold);
		checkCudaCall(cudaDeviceSynchronize());

		checkCudaCall(cudaMemcpy(h_counts, d_counts, iters * sizeof(int), cudaMemcpyDeviceToHost));
		checkCudaCall(cudaMemcpy(h_plane_params, d_plane_params, 4 * sizeof(float) * iters, cudaMemcpyDeviceToHost));

		for (int i = 0; i < iters; i++)
		{
			counts[i] = h_counts[i];
			plane_params[i * 4] = h_plane_params[i * 4];
			plane_params[i * 4 + 1] = h_plane_params[i * 4 + 1];
			plane_params[i * 4 + 2] = h_plane_params[i * 4 + 2];
			plane_params[i * 4 + 3] = h_plane_params[i * 4 + 3];
		}
		
		checkCudaCall(cudaGetLastError());

		checkCudaCall(cudaDeviceSynchronize());
	}
	catch (CudaException ex)
	{
		goto Error;
	}
	
Error:
	cudaStatus = cudaGetLastError();

	if(cublasH != NULL)
		cublasDestroy(cublasH);

	if(d_pts != 0)
		cudaFree(d_pts);

	if (d_plane_params != 0)
		cudaFree(d_plane_params);

	if (d_counts != 0)
		cudaFree(d_counts);

	cudaFree(d_As);
	cudaFree(d_bs);
	
	if (d_state != 0)
		cudaFree(d_state);

	free(h_bs);
	free(h_As);
	free(h_plane_params);
	free(h_counts);

	return cudaStatus;
}