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

void calculate_normals(std::vector<file_loader::vertex>& points, const float& init_threshold, const float& m_normal_range_steps, const int& m_normal_least_points, glm::vec3 viewpoint);

std::vector<file_loader::vertex> get_points_in_range(float r, const std::vector<file_loader::vertex>& points, const file_loader::vertex& target, const int& step, const int& least_points);

std::vector<int> get_points_in_range_by_index(float r, const std::vector<file_loader::vertex>& points, const file_loader::vertex& target);

glm::vec3 get_centroid(std::vector<file_loader::vertex> points);