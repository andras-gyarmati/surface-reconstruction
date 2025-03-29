#pragma once
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
    float* bestModel;
};

float* EstimatePlaneImplicit(const VertexSet& pts, const std::vector<int>& indices);

RANSACDiffs runRANSACPlane(const VertexSet& pts, const std::vector<int> indices, const int& iters, const float& threshold);