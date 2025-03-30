#pragma once
#include <vector>
#include "file_loader.h"
#include <iostream>
#include <Eigen/Dense>
#include <queue>

namespace supervoxel
{
	struct Voxel
	{
		glm::vec3 centroid;
		std::vector<file_loader::vertex*> points;

		glm::vec3 getColor() const;

		glm::vec3 getNormal() const;
	};

	std::unordered_map<int, Voxel> supervoxelSegmentation(
		std::vector<file_loader::vertex>& points,
		float voxel_resolution,
		float seed_resolution
	);
}

struct edge
{
	float color_w;
	float ransac_w;
	float distance_w;
	float normal_w;
	file_loader::vertex from;
	file_loader::vertex to;
	float accumulated_w;

	edge();

	float get_weight() const;
};

class vertex_graph
{
public:
	// std::vector<std::vector<edge>> mat;
	std::unordered_map<std::uint32_t, std::unordered_map<std::uint32_t, edge>> mat;
	std::unordered_map<std::uint32_t, supervoxel::Voxel> nodes;
	// std::vector<supervoxel::Voxel> nodes;
	std:unordered_map<std::uint32_t, std::uint32_t> adjacency
	

	vertex_graph() = default;

	vertex_graph(const std::vector<file_loader::vertex>& vertices);

	vertex_graph(const std::unordered_map<int, supervoxel::Voxel>& voxels);

	vertex_graph(const std::unordered_map<int, supervoxel::Voxel>& voxels, const std::vector<std::vector<int>>& groups, const std::vector<file_loader::vertex>& vertices);

	vertex_graph(const std::vector<file_loader::vertex>& vertices, const std::vector<std::vector<int>>& groups);

	//vertex_graph(const std::unordered_map<int, supervoxel::Voxel>& voxels, const std::vector<std::vector<int>>& groups);


	void print();

	std::vector<std::vector<int>> laplacian();

protected:

private:
};

namespace graph_utils
{
	//Minimal cost of cut
	float cut(vertex_graph v);

	float calc_weigth(file_loader::vertex a, file_loader::vertex b);

	void normalized_cut(const vertex_graph& input, vertex_graph& v1, vertex_graph& v2);
}

void spectral_cluster(std::vector<file_loader::vertex>& points, int iterations, float threshold);

void spectral_cluster(std::vector<file_loader::vertex>& points, int iterations, const std::vector<std::vector<int>>& groups);

glm::vec3 get_random_color();

glm::vec3 hsl_to_rgb(const float h, const float s, const float l);