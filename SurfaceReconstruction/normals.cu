#include "normals.h"
#define GPU

#pragma region CPU
std::vector<file_loader::vertex> get_points_in_range(float r, const std::vector<file_loader::vertex>& points, const file_loader::vertex& target, const int& step, const int& least_points) {
	std::vector<file_loader::vertex> in_range;

	for (auto& p : points) {
		if (glm::distance(p.position, target.position) < r) {
			in_range.push_back(p);
		}
	}

	return in_range;
}

std::vector<int> get_points_in_range_by_index(float r, const std::vector<file_loader::vertex>& points, const file_loader::vertex& target) {
	std::vector<int> in_range;
	int i = 0;
	for (auto& p : points) {
		if (glm::distance(p.position, target.position) < r) {
			in_range.push_back(i);
		}
		i++;
	}

	return in_range;
}

glm::vec3 get_centroid(std::vector<file_loader::vertex> points) {
	float x_sum, y_sum, z_sum;
	x_sum = z_sum = y_sum = 0.f;
	int n = points.size();

	for (const auto& p : points) {
		x_sum += p.position.x;
		y_sum += p.position.y;
		z_sum += p.position.z;
	}

	glm::vec3 centroid = { x_sum / n, y_sum / n, z_sum / n };;

	return centroid;
}
#pragma endregion

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

__device__ float distance_of_two_points(float* p_1, float* p_2)
{
	return sqrtf(powf(p_1[0] - p_2[0], 2) + powf(p_1[1] - p_2[1], 2) + powf(p_1[2] - p_2[2], 2));
}

__device__ int create_centroid_iterative(int id, float* pts, float* centroid, int pts_count, float& threshold, float range_step, int least_points)
{
	int c = 0;

	float current_point[3];
	current_point[0] = pts[id * 3];
	current_point[1] = pts[id * 3 + 1];
	current_point[2] = pts[id * 3 + 2];

	centroid[0] = 0.f;
	centroid[1] = 0.f;
	centroid[2] = 0.f;

	while(c < least_points)
	{
		for (int i = 0; i < pts_count; i++)
		{
			float p[3];
			p[0] = pts[i * 3];
			p[1] = pts[i * 3 + 1];
			p[2] = pts[i * 3 + 2];
			if (distance_of_two_points(p, current_point) < threshold)
			{
				centroid[0] = centroid[0] + p[0];
				centroid[1] = centroid[1] + p[1];
				centroid[2] = centroid[2] + p[2];
				c = c + 1;
			}
		}

		if (c < least_points)
		{
			threshold = threshold + range_step;
			c = 0;

			centroid[0] = 0.f;
			centroid[1] = 0.f;
			centroid[2] = 0.f;
		}
	}

	centroid[0] = centroid[0] / c;
	centroid[1] = centroid[1] / c;
	centroid[2] = centroid[2] / c;

	//printf("LAst one: %f\n", threshold);
	return threshold;
}

__global__ void write_normals(float* normals, float* eigenvectors)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;

	normals[id * 3] = eigenvectors[id * 9];
	normals[id * 3 + 1] = eigenvectors[id * 9 + 1];
	normals[id * 3 + 2] = eigenvectors[id * 9 + 2];
}

__global__ void create_systems_for_normals(float* pts, float* As, int pts_count, float threshold, float range_step, int least_points)
{
	int id = blockIdx.x * blockDim.x + threadIdx.x;

	float current_threshold = threshold;

	float current_point[3];
	current_point[0] = pts[id * 3];
	current_point[1] = pts[id * 3 + 1];
	current_point[2] = pts[id * 3 + 2];

	float centroid[3];
	create_centroid_iterative(id, pts, centroid, pts_count, current_threshold, range_step, least_points);

	float A[3][3];
	A[0][0] = 0.f;
	A[0][1] = 0.f;
	A[0][2] = 0.f;
	A[1][0] = 0.f;
	A[1][1] = 0.f;
	A[1][2] = 0.f;
	A[2][0] = 0.f;
	A[2][1] = 0.f;
	A[2][2] = 0.f;

	int count = 0;

	for (int i = 0; i < pts_count; i++)
	{
		float p[3];
		p[0] = pts[i * 3];
		p[1] = pts[i * 3 + 1];
		p[2] = pts[i * 3 + 2];

		if (distance_of_two_points(p, current_point) < current_threshold)
		{
			float ppT[3][3];
			ppT[0][0] = p[0] * p[0];
			ppT[0][1] = p[0] * p[1];
			ppT[0][2] = p[0] * p[2];
			ppT[1][0] = p[1] * p[0];
			ppT[1][1] = p[1] * p[1];
			ppT[1][2] = p[1] * p[2];
			ppT[2][0] = p[2] * p[0];
			ppT[2][1] = p[2] * p[1];
			ppT[2][2] = p[2] * p[2];

			float pcpcT[3][3];
			pcpcT[0][0] = centroid[0] * centroid[0];
			pcpcT[0][1] = centroid[0] * centroid[1];
			pcpcT[0][2] = centroid[0] * centroid[2];
			pcpcT[1][0] = centroid[1] * centroid[0];
			pcpcT[1][1] = centroid[1] * centroid[1];
			pcpcT[1][2] = centroid[1] * centroid[2];
			pcpcT[2][0] = centroid[2] * centroid[0];
			pcpcT[2][1] = centroid[2] * centroid[1];
			pcpcT[2][2] = centroid[2] * centroid[2];

			A[0][0] = A[0][0] + (ppT[0][0] - pcpcT[0][0]);
			A[0][1] = A[0][1] + (ppT[0][1] - pcpcT[0][1]);
			A[0][2] = A[0][2] + (ppT[0][2] - pcpcT[0][2]);
			A[1][0] = A[1][0] + (ppT[1][0] - pcpcT[1][0]);
			A[1][1] = A[1][1] + (ppT[1][1] - pcpcT[1][1]);
			A[1][2] = A[1][2] + (ppT[1][2] - pcpcT[1][2]);
			A[2][0] = A[2][0] + (ppT[2][0] - pcpcT[2][0]);
			A[2][1] = A[2][1] + (ppT[2][1] - pcpcT[2][1]);
			A[2][2] = A[2][2] + (ppT[2][2] - pcpcT[2][2]);

			count++;
		}
	}

	//Column major ordering for cuSolver
	As[id * 9] = A[0][0] / count;
	As[id * 9 + 1] = A[1][0] / count;
	As[id * 9 + 2] = A[2][0] / count;
	As[id * 9 + 3] = A[0][1] / count;
	As[id * 9 + 4] = A[1][1] / count;
	As[id * 9 + 5] = A[2][1] / count;
	As[id * 9 + 6] = A[0][2] / count;
	As[id * 9 + 7] = A[1][2] / count;
	As[id * 9 + 8] = A[2][2] / count;

	//printf("Point: %f, %f, %f; A matrix is: %f, %f, %f\n", current_point[0], current_point[1], current_point[2], A[0][0] / count, A[0][1] / count, A[0][2] / count, A[1][0] / count, A[1][1] / count, A[1][2] / count, A[2][0] / count, A[2][1], A[2][2] / count);
}

cudaError_t normalGPU(const std::vector<float>& pts, std::vector<float>& normals, const float& threshold, const float& range_step, const int& least_points, const int& threadNum)
{
	//std::cout << "Start with " << pts.size() << std::endl;

	float* d_pts = 0;
	float* d_As = 0;
	float* d_normals = 0;

	float* d_W = nullptr;
	int* d_info = nullptr;
	float* d_work = nullptr;
	int lwork = 0;

	cudaError_t cudaStatus;

	cublasHandle_t cublasH = NULL;

	try {
		checkCudaCall(cudaSetDevice(0));

		checkCudaCall(cudaMalloc((void**)&d_pts, pts.size() * sizeof(float)));
		checkCudaCall(cudaMalloc((void**)&d_As, pts.size() * 3 * sizeof(float)));
		checkCudaCall(cudaMalloc((void**)&d_normals, pts.size() * sizeof(float)));

		checkCudaCall(cudaMemcpy(d_pts, pts.data(), pts.size() * sizeof(float), cudaMemcpyHostToDevice));

		int threads = (pts.size() / 3);
		create_systems_for_normals <<< threads / threadNum, threadNum >>> (d_pts, d_As, pts.size() / 3, threshold, range_step, least_points);

		std::vector<float> A_checker(pts.size() * 3);
		checkCudaCall(cudaMemcpy(A_checker.data(), d_As, pts.size() * 3 * sizeof(float), cudaMemcpyDeviceToHost));

		//////////////////
		cusolverDnHandle_t cusolverH = NULL;
		cudaStream_t stream = NULL;
		syevjInfo_t syevj_params = NULL;

		const int m = 3;
		const int lda = 3;
		const int batchSize = pts.size() / 3;

		std::vector<float> V(lda * m * (pts.size() / 3), 0);
		std::vector<float> W(m * (pts.size() / 3), 0);
		std::vector<int> info(batchSize, 0);

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
		cudaMemcpyAsync(V.data(), d_As, sizeof(float) * 9 * (pts.size() / 3), cudaMemcpyDeviceToHost, stream);
		cudaMemcpyAsync(W.data(), d_W, sizeof(float) * 3 * (pts.size() / 3), cudaMemcpyDeviceToHost, stream);
		cudaStreamSynchronize(stream);
		/////////////////

		write_normals <<< threads / threadNum, threadNum >>> (d_normals, d_As);
		checkCudaCall(cudaDeviceSynchronize());

		checkCudaCall(cudaMemcpy(normals.data(), d_normals, pts.size() * sizeof(float), cudaMemcpyDeviceToHost));

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

	if (d_As != 0)
		cudaFree(d_As);

	if (d_W != 0)
		cudaFree(d_W);

	if (d_info != 0)
		cudaFree(d_info);

	if (d_work != 0)
		cudaFree(d_work);

	return cudaStatus;
}

#pragma endregion

void calculate_normals(std::vector<file_loader::vertex>& points, const float& init_threshold, const float& m_normal_range_steps, const int& m_normal_least_points, glm::vec3 viewpoint)
{
#ifndef GPU
	for (auto& p : points) {
		std::vector<file_loader::vertex> in_range = get_points_in_range(init_threshold, points, p, m_normal_range_steps, m_normal_least_points);
		std::vector<int> in_range_ind = get_points_in_range_by_index(init_threshold, points, p);

		glm::vec3 centroid = get_centroid(in_range);

		//printf("Point: %f, %f, %f; Centroid inside function: %f, %f, %f\n", p.position.x, p.position.y, p.position.z, centroid.x, centroid.y, centroid.z);

		
		Eigen::Matrix3f M;
		M.setZero();
		for (auto& p2 : in_range)
		{
			Eigen::Vector3f p_minus_centroid = Eigen::Vector3f(p2.position.x, p2.position.y, p2.position.z) - Eigen::Vector3f(centroid.x, centroid.y, centroid.z);
			M += p_minus_centroid * p_minus_centroid.transpose();
		}

		M /= in_range.size();
		//printf("Point: %f, %f, %f; M matrix is: %f, %f, %f\n", p.position.x, p.position.y, p.position.z, M.row(0)[0], M.row(0)[1], M.row(0)[2], M.row(1)[0], M.row(1)[1], M.row(1)[2], M.row(2)[0], M.row(2)[1], M.row(2)[2]);

		Eigen::EigenSolver<Eigen::Matrix3f> es(M);
		const int lowestEigenValueIndex = std::min({ 0,1,2 },
			[&es](int v1, int v2) {
				return es.eigenvalues()[v1].real() < es.eigenvalues()[v2].real();
			});
		float A = es.eigenvectors().col(lowestEigenValueIndex)(0).real();
		float B = es.eigenvectors().col(lowestEigenValueIndex)(1).real();
		float C = es.eigenvectors().col(lowestEigenValueIndex)(2).real();
		glm::vec3 p_normal(A, B, C);

		if (glm::dot(p_normal, (-p.position)) <= 0)
		{
			p_normal = -p_normal;
		}

		p.normal = p_normal;
}

#endif // !GPU
	
#ifdef GPU
	std::vector<float> coords;
	for (auto& p : points) {
		coords.emplace_back(p.position.x);
		coords.emplace_back(p.position.y);
		coords.emplace_back(p.position.z);
	}

	std::vector<float> normals;
	normals.resize(points.size() * 3);

	cudaError_t cudaStatus = normalGPU(coords, normals, init_threshold, m_normal_range_steps, m_normal_least_points, 256);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "normalGPU failed!");
	}

	for (int i = 0; i < points.size(); i++) {
		glm::vec3 normal = glm::vec3(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);
		glm::vec3 toCamera = -(points[i].position);
		if (glm::dot(normal, (toCamera)) < 0.01f)
		{
			normal = -normal;
		}

		points[i].normal = normal;
	}

	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceReset failed!");
	}

#endif // GPU
}