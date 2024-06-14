#include "file_loader.h"
#include "VertexSet.h"
#define EIGEN_NO_CUDA
#include "Eigen/Dense"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "cusolverDn.h"
#include "curand.h"
#include "curand_kernel.h"
#include <exception>
#include <random>

struct RANSACDiffs {
    int inliersNum;
    std::vector<bool> isInliers;
    std::vector<float> distances;
};

#pragma region GPU

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

	float model[4] = { plane_params[id * 4] , plane_params[id * 4 + 1] , plane_params[id * 4 + 2], plane_params[id * 4 + 3] };

	int count = 0;
	for (int i = 0; i < size; i++)
	{
		float p[3] = { pts[i * 3], pts[i * 3 + 1], pts[i * 3 + 2] };
		if (distance(model, p) < threshold)
			count++;
	}

	count_inliers[id] = count;
}

__global__ void createSystems(float* pts, float* As, int seed, curandState* rand_state, int size)
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

	float At[4][3];
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			At[i][j] = Aorig[j][i];
		}
	}

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			As[id * 16 + j * 4 + i] = 0;

			for (int k = 0; k < 3; k++) {
				//Column major ordering
				As[id * 16 + j * 4 + i] += At[i][k] * Aorig[k][j];
			}
		}
	}
}

cudaError_t runRansacGPU(const std::vector<float>& pts, std::vector<float>& plane_params, std::vector<int>& counts, const int& iters, const float& threshold, const int& threadNum)
{
	float* d_pts = 0;
	float* d_plane_params = 0;
	int* d_counts = 0;
	float* d_As = 0;

	int* h_counts = (int*)malloc(sizeof(int) * iters);
	float* h_plane_params = (float*)malloc(sizeof(float) * 4 * iters);

	curandState* d_state;
	cudaError_t cudaStatus;

	cublasHandle_t cublasH = NULL;

	try {
		checkCudaCall(cudaSetDevice(0));

		checkCudaCall(cudaMalloc((void**)&d_pts, pts.size() * sizeof(float)));
		checkCudaCall(cudaMalloc((void**)&d_plane_params, iters * 4 * sizeof(float)));
		checkCudaCall(cudaMalloc((void**)&d_counts, iters * sizeof(float)));

		checkCudaCall(cudaMalloc(&d_state, iters * sizeof(curandState)));
		checkCudaCall(cudaMalloc((void**)&d_As, iters * 16 * sizeof(float)));

		checkCudaCall(cudaMemcpy(d_pts, pts.data(), pts.size() * sizeof(float), cudaMemcpyHostToDevice));

		srand(time(0));
		int seed = rand();

		createSystems <<< iters / threadNum, threadNum >>> (d_pts, d_As, seed, d_state, pts.size() / 3);

		//////////////////
		cusolverDnHandle_t cusolverH = NULL;
		cudaStream_t stream = NULL;
		syevjInfo_t syevj_params = NULL;

		const int m = 4;
		const int lda = 4;
		const int batchSize = iters;

		std::vector<float> V(lda * m * iters, 0);
		std::vector<float> W(m * iters, 0);
		std::vector<int> info(batchSize, 0);

		float* d_W = nullptr;
		int* d_info = nullptr;
		float* d_work = nullptr;
		int lwork = 0;

		/* configuration of syevj  */
		const double tol = 1.e-7;
		const int max_sweeps = 15;
		const int sort_eig = 1;                           /* sort eigenvalues/vectors in ascending order */
		const cusolverEigMode_t jobz = CUSOLVER_EIG_MODE_VECTOR; /* compute eigenvectors */
		const cublasFillMode_t uplo = CUBLAS_FILL_MODE_LOWER;

		cusolverDnCreate(&cusolverH);
		cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking);
		cusolverDnSetStream(cusolverH, stream);
		cusolverDnCreateSyevjInfo(&syevj_params);
		cusolverDnXsyevjSetTolerance(syevj_params, tol);
		cusolverDnXsyevjSetMaxSweeps(syevj_params, max_sweeps);
		cusolverDnXsyevjSetSortEig(syevj_params, sort_eig);

		cudaMalloc((void**)&d_W, sizeof(float) * W.size());
		cudaMalloc((void**)&d_info, sizeof(int) * info.size());
		cusolverDnSsyevjBatched_bufferSize(cusolverH, jobz, uplo, m, d_As, lda, d_W, &lwork, syevj_params, batchSize);
		cudaMalloc(reinterpret_cast<void**>(&d_work), sizeof(double) * lwork);
		cusolverDnSsyevjBatched(cusolverH, jobz, uplo, m, d_As, lda, d_W, d_work, lwork, d_info, syevj_params, batchSize);
		cudaMemcpyAsync(V.data(), d_As, sizeof(float) * 16 * iters, cudaMemcpyDeviceToHost, stream);
		cudaMemcpyAsync(W.data(), d_W, sizeof(float) * 4 * iters, cudaMemcpyDeviceToHost, stream);
		cudaStreamSynchronize(stream);
		/////////////////

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

	if (cublasH != NULL)
		cublasDestroy(cublasH);

	if (d_pts != 0)
		cudaFree(d_pts);

	if (d_plane_params != 0)
		cudaFree(d_plane_params);

	if (d_counts != 0)
		cudaFree(d_counts);

	if (d_As != 0)
		cudaFree(d_As);

	if (d_state != 0)
		cudaFree(d_state);

	free(h_plane_params);
	free(h_counts);

	return cudaStatus;
}

#pragma endregion


#pragma region CPU

float* EstimatePlaneImplicit(const VertexSet& pts, const std::vector<int>& indices) {
    const size_t num = indices.size();

    Eigen::MatrixXf Cfs(num, 4);

    for (size_t i = 0; i < num; i++) {
        file_loader::vertex pt = pts[indices[i]];
        Cfs(i, 0) = pt.position.x;
        Cfs(i, 1) = pt.position.y;
        Cfs(i, 2) = pt.position.z;
        Cfs(i, 3) = 1.0f;
    }

    Eigen::MatrixXf mtx = Cfs.transpose() * Cfs;
    Eigen::EigenSolver<Eigen::MatrixXf> es(mtx);

    const int lowestEigenValueIndex = std::min({ 0,1,2,3 },
        [&es](int v1, int v2) {
            return es.eigenvalues()[v1].real() < es.eigenvalues()[v2].real();
        });

    float A = es.eigenvectors().col(lowestEigenValueIndex)(0).real();
    float B = es.eigenvectors().col(lowestEigenValueIndex)(1).real();
    float C = es.eigenvectors().col(lowestEigenValueIndex)(2).real();
    float D = es.eigenvectors().col(lowestEigenValueIndex)(3).real();

    float norm = std::sqrt(A * A + B * B + C * C);

    float* ret = new float[4];
    ret[0] = A / norm;
    ret[1] = B / norm;
    ret[2] = C / norm;
    ret[3] = D / norm;

    return ret;
}

RANSACDiffs PlanePointRANSACDifferences(const VertexSet& pts, const std::vector<int>& indices, float* plane, float threshold) {
    size_t num = indices.size();

    float A = plane[0];
    float B = plane[1];
    float C = plane[2];
    float D = plane[3];

    RANSACDiffs ret;

    std::vector<bool> isInliers;
    std::vector<float> distances;

    int inlierCounter = 0;
    for (int idx = 0; idx < num; idx++) {
        file_loader::vertex pt = pts[indices[idx]];
        float diff = fabs(A * pt.position.x + B * pt.position.y + C * pt.position.z + D);
        distances.push_back(diff);
        if (diff < threshold) {
            isInliers.push_back(true);
            ++inlierCounter;
        }
        else {
            isInliers.push_back(false);
        }
    }

    ret.distances = distances;
    ret.isInliers = isInliers;
    ret.inliersNum = inlierCounter;

    return ret;
}

float* EstimatePlaneRANSAC(const VertexSet& pts, const std::vector<int>& indices, float threshold, int iterNum) {
    size_t num = indices.size();

    int bestSampleInlierNum = 0;
    float* bestPlane = new float[4];

    for (size_t iter = 0; iter < iterNum; iter++) {
        int index1 = rand() % num;
        int index2 = rand() % num;

        while (index2 == index1) {
            index2 = rand() % num;
        }
        int index3 = rand() % num;
        while (index3 == index1 || index3 == index2) {
            index3 = rand() % num;
        }

        const std::vector<int> minimalSample = { index1, index2, index3 };

        float* samplePlane = EstimatePlaneImplicit(pts, minimalSample);

        RANSACDiffs sampleResult = PlanePointRANSACDifferences(pts, indices, samplePlane, threshold);

        if (sampleResult.inliersNum > bestSampleInlierNum) {
            bestSampleInlierNum = sampleResult.inliersNum;
            for (int i = 0; i < 4; ++i) {
                bestPlane[i] = samplePlane[i];
            }
        }

        delete[] samplePlane;
    }

    RANSACDiffs bestResult = PlanePointRANSACDifferences(pts, indices, bestPlane, threshold);
    std::cout << "Best plane params: " << bestPlane[0] << " " << bestPlane[1] << " " << bestPlane[2] << "\n";

    std::vector<int> inlierPts;

    for (int idx = 0; idx < num; idx++) {
        if (bestResult.isInliers.at(idx)) {
            inlierPts.push_back(indices[idx]);
        }
    }

    float* finalPlane = EstimatePlaneImplicit(pts, inlierPts);
    delete[] bestPlane;
    return finalPlane;
}

#pragma endregion

RANSACDiffs runRANSACPlane(const VertexSet& pts, const std::vector<int> indices, const int& iters, const float& threshold)
{
    float* bestModel;

#ifdef GPU

    std::vector<float> input(indices.size() * 3);
    std::vector<float> plane_params(iters * 4);
    std::vector<int> count(iters);

    for (size_t j = 0; j < indices.size(); j++)
    {
        input[j * 3] = pts[indices[j]].position.x;
        input[j * 3 + 1] = pts[indices[j]].position.y;
        input[j * 3 + 2] = pts[indices[j]].position.z;

        //std::cout << m_vertices[filteredPoints[j]].position.x << " " << m_vertices[filteredPoints[j]].position.y << m_vertices[filteredPoints[j]].position.z << "\n";
    }

    cudaError_t cudaStatus = runRansacGPU(input, plane_params, count, iters, threshold, 256);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "runRansacGPU failed!");
    }

    int mostInlierIndex = 0;
    for (int i = 0; i < iters; i++)
    {
        if (count[i] > count[mostInlierIndex])
            mostInlierIndex = i;
    }

    std::cout << count[mostInlierIndex] << "\n";

    bestModel = new float[4];
    bestModel[0] = plane_params[mostInlierIndex * 4];
    bestModel[1] = plane_params[mostInlierIndex * 4 + 1];
    bestModel[2] = plane_params[mostInlierIndex * 4 + 2];
    bestModel[3] = plane_params[mostInlierIndex * 4 + 3];

    cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceReset failed!");
    }

#endif // GPU

#ifndef GPU

    bestModel = EstimatePlaneRANSAC(pts, indices, threshold, iters);

#endif // !GPU

    size_t num = indices.size();

    std::cout << "Plane params RANSAC:" << std::endl;
    std::cout << "A:" << bestModel[0] << " B:" << bestModel[1]
        << " C:" << bestModel[2] << " D:" << bestModel[3] << std::endl;

    RANSACDiffs differences = PlanePointRANSACDifferences(pts ,indices, bestModel, threshold);
    delete[] bestModel;

    return differences;
}