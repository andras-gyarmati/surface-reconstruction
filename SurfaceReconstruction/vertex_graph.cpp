#pragma once
#include "vertex_graph.h"
#include <fstream>
#include "octree.h"
#include <glm/glm.hpp>
#include <pcl/console/parse.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/segmentation/supervoxel_clustering.h>

namespace supervoxel
{
	glm::vec3 Voxel::getColor() const
	{
		glm::vec3 c(0, 0, 0);

		for (auto p : points)
		{
			c += p->color;
		}

		//std::cout << c.x << ", " << c.y << ", " << c.z << "\n";

		glm::vec3 tmp = c * (1.f / points.size());
		return tmp;
	}

	glm::vec3 Voxel::getNormal() const
	{
		glm::vec3 n(0, 0, 0);

		for (auto& p : points)
		{
			n += p->normal;
		}

		return glm::normalize(n);
	}

	std::unordered_map<int, Voxel> supervoxelSegmentation
	(
		std::vector<file_loader::vertex>& points,
		float voxel_resolution,
		float seed_resolution,
		float color_importance,
		float spatial_importance,
		float normal_importance
	) 
	{

	// Step 1: Create a voxel grid
		std::unordered_map<int, Voxel> voxels;
		for (auto& point : points) {
			int grid_x = static_cast<int>(std::floor(point.position.x / voxel_resolution));
			int grid_y = static_cast<int>(std::floor(point.position.y / voxel_resolution));
			int grid_z = static_cast<int>(std::floor(point.position.z / voxel_resolution));

			int voxel_id = grid_x * 73856093 ^ grid_y * 19349663 ^ grid_z * 83492791;
			voxels[voxel_id].points.push_back(&point);
		}

		// Step 2: Compute voxel centroids
		for (auto& [id, voxel] : voxels) {
			float x_sum = 0, y_sum = 0, z_sum = 0;
			for (const auto& p : voxel.points) {
				x_sum += p->position.x;
				y_sum += p->position.y;
				z_sum += p->position.z;
			}
			size_t n = voxel.points.size();
			voxel.centroid = { x_sum / n, y_sum / n, z_sum / n };
		}

		// Step 3: Seed supervoxels
		std::vector<glm::vec3> seeds;
		for (const auto& [id, voxel] : voxels) {
			if (static_cast<float>(rand()) / RAND_MAX < seed_resolution) {
				seeds.push_back(voxel.centroid);
			}
		}

		// Step 4: Assign points to supervoxels
		std::unordered_map<int, Voxel> supervoxels;
		for (auto& point : points) {
			float min_distance = std::numeric_limits<float>::max();
			int closest_seed = -1;
			for (size_t i = 0; i < seeds.size(); ++i) {
				float dist = glm::distance(point.position, seeds[i]);
				if (dist < min_distance) {
					min_distance = dist;
					closest_seed = i;
				}
			}
			supervoxels[closest_seed].points.push_back(&point);
		}

		// Step 5: Recompute centroids for supervoxels
		for (auto& [id, supervoxel] : supervoxels) {
			float x_sum = 0, y_sum = 0, z_sum = 0;
			for (const auto& p : supervoxel.points) {
				x_sum += p->position.x;
				y_sum += p->position.y;
				z_sum += p->position.z;
			}
			size_t n = supervoxel.points.size();
			supervoxel.centroid = { x_sum / n, y_sum / n, z_sum / n };
		}

		return supervoxels;
	}

	std::unordered_map<int, Voxel> supervoxelSegmentation
	(
		std::vector<file_loader::vertex*>& points,
		float voxel_resolution,
		float seed_resolution
	)
	{
		// Step 1: Create a voxel grid
		std::unordered_map<int, Voxel> voxels;
		for (auto& point : points) {
			int grid_x = static_cast<int>(std::floor(point->position.x / voxel_resolution));
			int grid_y = static_cast<int>(std::floor(point->position.y / voxel_resolution));
			int grid_z = static_cast<int>(std::floor(point->position.z / voxel_resolution));

			int voxel_id = grid_x * 73856093 ^ grid_y * 19349663 ^ grid_z * 83492791;
			voxels[voxel_id].points.push_back(point);
		}

		// Step 2: Compute voxel centroids
		for (auto& [id, voxel] : voxels) {
			float x_sum = 0, y_sum = 0, z_sum = 0;
			for (const auto& p : voxel.points) {
				x_sum += p->position.x;
				y_sum += p->position.y;
				z_sum += p->position.z;
			}
			size_t n = voxel.points.size();
			voxel.centroid = { x_sum / n, y_sum / n, z_sum / n };
		}

		// Step 3: Seed supervoxels
		std::vector<glm::vec3> seeds;
		for (const auto& [id, voxel] : voxels) {
			if (static_cast<float>(rand()) / RAND_MAX < seed_resolution) {
				seeds.push_back(voxel.centroid);
			}
		}

		// Step 4: Assign points to supervoxels
		std::unordered_map<int, Voxel> supervoxels;
		for (auto& point : points) {
			float min_distance = std::numeric_limits<float>::max();
			int closest_seed = -1;
			for (size_t i = 0; i < seeds.size(); ++i) {
				float dist = glm::distance(point->position, seeds[i]);
				if (dist < min_distance) {
					min_distance = dist;
					closest_seed = i;
				}
			}
			supervoxels[closest_seed].points.push_back(point);
		}

		// Step 5: Recompute centroids for supervoxels
		for (auto& [id, supervoxel] : supervoxels) {
			float x_sum = 0, y_sum = 0, z_sum = 0;
			for (const auto& p : supervoxel.points) {
				x_sum += p->position.x;
				y_sum += p->position.y;
				z_sum += p->position.z;
			}
			size_t n = supervoxel.points.size();
			supervoxel.centroid = { x_sum / n, y_sum / n, z_sum / n };
		}

		return supervoxels;
	}
}

edge::edge()
{
	color_w = 0;
	ransac_w = 0;
	distance_w = 0;
	accumulated_w = 0;
}

float edge::get_weight() const
{
	//return color_w + ransac_w + distance_w;
	return accumulated_w;
}

vertex_graph::vertex_graph(const std::vector<file_loader::vertex>& vertices)
{
	const size_t num = vertices.size();
	mat = std::vector<std::vector<edge>>(num, std::vector<edge>(num, edge()));

	typedef pcl::PointXYZRGBNormal PointT;
	typedef pcl::PointCloud<PointT> PointCloudT;
	typedef pcl::PointNormal PointNT;
	typedef pcl::PointCloud<PointNT> PointNCloudT;
	typedef pcl::PointXYZL PointLT;
	typedef pcl::PointCloud<PointLT> PointLCloudT;

	PointCloudT::Ptr cloud(new PointCloudT);
	for(auto& p : points)
	{
		PointT tmp(p.position.x, p.position.y, p.position.z,
			0,0,0,
			p.normal.x, p.normal.y, p.normal.z);

		std::uint8_t r = p.color.x, g = p.color.y, b = p.color.z;
		std::uint32_t rgb = ((std::uint32_t)r << 16 | (std::uint32_t)g << 8 | (std::uint32_t)b);
		tmp.rgb = *reinterpret_cast<float*>(&rgb);

		tmp.push_back(tmp);
	}
	
	pcl::SupervoxelClustering<PointT> super(voxel_resolution, seed_resolution);
	super.setInputCloud(cloud);
	super.setColorImportance(color_importance);
	super.setSpatialImportance(spatial_importance);
	super.setNormalImportance(normal_importance);
	std::map <std::uint32_t, pcl::Supervoxel<PointT>::Ptr> supervoxel_clusters;
	for(auto& c : supervoxel_clusters)
	{
		if(nodes.find(c.first) == nodes.end())
		{
			nodes[c.first] = Supervoxel::voxel();
			PointCloudT points_of_supervoxel = c.second.voxels_;
			for (auto p : points_of_supervoxel)
			{
				glm::vec3 cluster_pos = glm::vec3(p.x, p.y, p.z);
				for(auto& tmp : vertices)
				{
					if(cluster_pos == tmp.position)
						nodes[c.first].points.push_back(tmp)
				}
			}
		}
	}

	super.extract (supervoxel_clusters);

	pcl::visualization::PCLVisualizer::Ptr viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));
	viewer->setBackgroundColor (0, 0, 0);
	PointLCloudT::Ptr labeled_voxel_cloud = super.getLabeledVoxelCloud ();
	viewer->addPointCloud (labeled_voxel_cloud, "labeled voxels");
	viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_OPACITY,0.8, "labeled voxels");	  

	std::multimap<std::uint32_t, std::uint32_t> supervoxel_adjacency;
	super.getSupervoxelAdjacency (supervoxel_adjacency);
	for (auto label_itr = supervoxel_adjacency.cbegin (); label_itr != supervoxel_adjacency.cend (); )
	{
		//First get the label
		std::uint32_t supervoxel_label = label_itr->first;
		//Now get the supervoxel corresponding to the label
		pcl::Supervoxel<PointT>::Ptr supervoxel = supervoxel_clusters.at(supervoxel_label);

		//Now we need to iterate through the adjacent supervoxels and make a point cloud of them
		PointCloudT adjacent_supervoxel_centers;
	
		for (auto adjacent_itr = supervoxel_adjacency.equal_range (supervoxel_label).first; adjacent_itr!=supervoxel_adjacency.equal_range (supervoxel_label).second; ++adjacent_itr)
		{
			edge e;
			e.accumulated_w = graph_utils::calc_weigth(nodes[supervoxel_label], nodes[adjacent_itr]);
			mat[supervoxel_label][adjacent_itr] = mat[adjacent_itr][supervoxel_label] = e;
			// pcl::Supervoxel<PointT>::Ptr neighbor_supervoxel = supervoxel_clusters.at (adjacent_itr->second);
			// adjacent_supervoxel_centers.push_back (neighbor_supervoxel->centroid_);
		}
	
		//Now we make a name for this polygon
		std::stringstream ss;
		ss << "supervoxel_" << supervoxel_label;
		//This function is shown below, but is beyond the scope of this tutorial - basically it just generates a "star" polygon mesh from the points given
		addSupervoxelConnectionsToViewer (supervoxel->centroid_, adjacent_supervoxel_centers, ss.str (), viewer);
		//Move iterator forward to next label
		label_itr = supervoxel_adjacency.upper_bound (supervoxel_label);
	}

	//TODO
	// const size_t num = vertices.size();

	// mat = std::vector<std::vector<edge>>(num, std::vector<edge>(num, edge()));

	// for (size_t i = 0; i < num; i++)
	// {
	// 	for (size_t j = 0; j <= i; j++)
	// 	{
	// 		edge e;
	// 		e.from = vertices[i];
	// 		e.to = vertices[j];
	// 		e.color_w = glm::distance(vertices[i].color, vertices[j].color) / 442.f;
	// 		e.ransac_w = 0;
	// 		e.distance_w = (glm::distance(vertices[i].position, vertices[j].position) <= 10.f) ? 1 : 0;

	// 		e.accumulated_w = graph_utils::calc_weigth(vertices[i], vertices[j]);

	// 		mat[j][i] = mat[i][j] = e;
	// 	}
	// }
}

vertex_graph::vertex_graph(const std::vector<file_loader::vertex>& vertices, const std::vector<std::vector<int>>& groups)
{
	const size_t num = vertices.size();

	mat = std::vector<std::vector<edge>>(num, std::vector<edge>(num, edge()));

	for (size_t i = 0; i < num; i++)
	{
		for (size_t j = 0; j <= i; j++)
		{
			edge e;
			e.color_w = glm::distance(vertices[i].color, vertices[j].color) / 442.f;
			e.ransac_w = 0;
			e.from = vertices[i];
			e.from = vertices[j];
			for (auto& g : groups)
			{
				bool found_one = false;
				bool found_two = false;

				for (auto& idx : g)
				{
					if (&vertices[i] == &vertices[idx])
						found_one = true;
					if (&vertices[j] == &vertices[idx])
						found_two = true;

					if (found_one && found_two)
					{
						e.ransac_w = 1;
						break;
					}
				}

				if (found_one == found_two == false)
					break;
			}
			//bool found_one = false;
			//bool found_two = false;
			//while (!found_one && !found_two)
			//{
			//	found_two = found_one = false;
			//	for (auto& g : groups)
			//	{
			//		for (auto& idx : g)
			//		{
			//			if (&vertices[idx] == &vertices[i])
			//				found_one = true;
			//			if (&vertices[idx] == &vertices[j])
			//				found_two = true;
			//		}
			//	}
			//}

			//if (found_one && found_two)
			//{
			//	e.ransac_w = 1;
			//}
			//else
			//{
			//	e.ransac_w = 0;
			//}

			e.distance_w = (glm::distance(vertices[i].position, vertices[j].position) <= 10.f) ? 1 : 0;
			e.accumulated_w = graph_utils::calc_weigth(vertices[i], vertices[j]);

			mat[j][i] = mat[i][j] = e;
		}
	}
}

vertex_graph::vertex_graph(const std::unordered_map<int, supervoxel::Voxel>& voxels)
{
	const size_t num = voxels.size();

	mat = std::vector<std::vector<edge>>(num, std::vector<edge>(num, edge()));

	int i = 0;
	int j = 0;
	for (auto& voxel_i : voxels)
	{
		nodes.push_back(voxel_i.second);
		j = 0;
		for (auto& voxel_j : voxels)
		{
			edge e;
			glm::vec3 v_i_color = voxel_i.second.getColor();
			
			file_loader::vertex tmp1;
			tmp1.normal = voxel_i.second.getNormal();
			tmp1.position = voxel_i.second.centroid;
			file_loader::vertex tmp2;
			tmp2.normal = voxel_j.second.getNormal();
			tmp2.position = voxel_j.second.centroid;
			e.accumulated_w = graph_utils::calc_weigth(tmp1, tmp2);

			glm::vec3 v_j_color = voxel_j.second.getColor();
			e.color_w = glm::distance(v_i_color, v_j_color); /// 442.f;
			e.ransac_w = 0;
			e.distance_w = (glm::distance(voxel_i.second.centroid, voxel_j.second.centroid) <= 10.f) ? 1 : 0;

			mat[i][j] = e;
			j++;
		}
		i++;
	}
}

vertex_graph::vertex_graph(const std::unordered_map<int, supervoxel::Voxel>& voxels, const std::vector<std::vector<int>>& groups, const std::vector<file_loader::vertex>& vertices)
{
	const size_t num = voxels.size();

	mat = std::vector<std::vector<edge>>(num, std::vector<edge>(num, edge()));

	int i = 0;
	int j = 0;
	for (auto& voxel_i : voxels)
	{
		nodes.push_back(voxel_i.second);
		j = 0;
		for (auto& voxel_j : voxels)
		{
			edge e;
			e.color_w = glm::distance(voxel_i.second.getColor(), voxel_j.second.getColor()) / 442.f;
			e.ransac_w = 0;
			for (auto& p_i : voxel_i.second.points)
			{
				for (auto& p_j : voxel_j.second.points)
				{
					for (auto& g : groups)
					{
						bool found_one = false;
						bool found_two = false;

						for (auto& idx : g)
						{
							if (p_i == &vertices[idx])
								found_one = true;
							if (p_j == &vertices[idx])
								found_two = true;

							if (found_one && found_two)
							{
								e.ransac_w += 1;
								continue;
							}
						}
					}
				}
			}

			e.ransac_w /= (voxel_i.second.points.size() * voxel_j.second.points.size());
			e.distance_w = (glm::distance(voxel_i.second.centroid, voxel_j.second.centroid) <= 10.f) ? 1 : 0;

			file_loader::vertex tmp1;
			tmp1.normal = voxel_i.second.getNormal();
			tmp1.position = voxel_i.second.centroid;
			file_loader::vertex tmp2;
			tmp1.normal = voxel_j.second.getNormal();
			tmp1.position = voxel_j.second.centroid;
			e.accumulated_w = graph_utils::calc_weigth(tmp1, tmp2);

			mat[i][j] = e;
			j++;
		}
		i++;
	}
}

void vertex_graph::print()
{
	std::cout << "Graph: \n";

	int V = mat.size();
	for (int i = 0; i < V; i++)
	{
		for (int j = 0; j < V; j++)
			std::cout << "(" << mat[i][j].color_w << "|" << mat[i][j].ransac_w << "|" << mat[i][j].distance_w << ")";
		std::cout << std::endl;
	}
}

std::vector<std::vector<int>> vertex_graph::laplacian() {
	const size_t num = mat.size();

	std::vector<std::vector<int>> L;
	L.reserve(num);

	for (size_t i = 0; i < num; i++)
	{
		for (size_t j = 0; j < num; j++)
		{
			if (i == j) { L[i][j] = num; }
			else { L[i][j] = -mat[i][j].get_weight(); }
		}
	}

	return L;
}

//Minimal cost of cut
namespace graph_utils
{
	float cut(vertex_graph v) {
		return 1.f;
	}


	float flux(float r) {
		//TODO
		return glm::exp(-(r * r) / 1);
	}
	
	float calc_weigth(file_loader::vertex a, file_loader::vertex b)
	{
		// Gives angle between normals in radians
		float angle = glm::asin(glm::dot(a.normal, b.normal) / (glm::length(a.normal * glm::length(b.normal))));
		// Convert to degrees for me poor human eyes:)
		angle = angle / glm::pi<float>() * 180.f;
		angle = glm::abs(angle);
		//float angle = glm::asin(glm::dot(a.position, b.position) / (glm::length(a.position * glm::length(b.position))));
		//std::cout << a.normal.x << a.normal.y << a.normal.z << " to " << b.normal.x << b.normal.y << b.normal.z << " angle: " << angle << std::endl;
		//plane case
		if (angle < 10.f)
			return 1.f;

		if (glm::dot((b.normal - a.normal), (b.position - a.position)) > 0.f && (glm::dot((b.position - a.position) - (glm::dot((b.position - a.position), b.normal)), a.normal) > 0.f))
			return 1.f;

		return 0.f;
	}

	void normalized_cut(const vertex_graph& input, vertex_graph& v1, vertex_graph& v2) {
		const size_t num = input.mat.size();

		Eigen::MatrixXf D(num, num);
		Eigen::MatrixXf W(num, num);

		//Create D diagonal matrix and W symmetric matrix and L laplacian matrix
		for (size_t i = 0; i < num; i++)
		{
			for (size_t j = 0; j < num; j++)
			{
				if (i == j)
				{
					D(i, j) = static_cast<float>(num);
					W(i, j) = 0.f;
				}
				else
				{
					D(i, j) = 0.f;
					W(i, j) = input.mat[i][j].get_weight();

				}
			}
		}
		//std::ofstream dout("D.txt");
		//std::ofstream wout("W.txt");
		//std::ofstream lout("L.txt");
		//dout << D << "\n";
		//wout << W << "\n";
		Eigen::MatrixXf L = D - W;
		//lout << L << "\n";

		//Solve generalized linear model
		Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXf> ges;
		ges.compute(L, D);

		//Get Fiedler vector (eigenvector of second smallest eigenvalue)
		int smallest = 0;
		int second_smallest = 1;
		int tmp;
		if (ges.eigenvalues()[smallest] > ges.eigenvalues()[second_smallest])
		{
			tmp = smallest;
			smallest = second_smallest;
			second_smallest = tmp;
		}

		for (int i = 2; i < ges.eigenvalues().size(); i++)
		{
			if (ges.eigenvalues()[i] < ges.eigenvalues()[smallest])
			{
				second_smallest = smallest;
				smallest = i;
			}
			else if (ges.eigenvalues()[i] < ges.eigenvalues()[second_smallest])
			{
				second_smallest = i;
			}
		}

		auto eigenvector = ges.eigenvectors().col(second_smallest);
		//this eigenvector is the indicator vector for our cut
		//based on sign of the vectors value we create 2 separate graphs

		//std::ofstream eigenout("eigenvector.txt");
		//eigenout << eigenvector << "\n";

		std::vector<file_loader::vertex*> out_1;
		std::vector<file_loader::vertex*> out_2;
		for (int i = 0; i < input.nodes.size(); i++)
		{
			auto curr = input.nodes[i];
			for (auto v : curr.points)
			{
				if (eigenvector[i] < 0)
				{
					out_1.push_back(v);
				}
				else
				{
					out_2.push_back(v);
				}
			}
		}
		v1 = vertex_graph(supervoxel::supervoxelSegmentation(out_1, 0.5, 1));
		v2 = vertex_graph(supervoxel::supervoxelSegmentation(out_2, 0.5, 1));
	}
}

void spectral_cluster(std::vector<file_loader::vertex>& points, int iterations, const std::vector<std::vector<int>>& groups)
{
	auto supervoxels = supervoxel::supervoxelSegmentation(points, 0.5, 1);

	auto m_graph = vertex_graph(supervoxels, groups, points);
	std::queue<vertex_graph> graph_queue;
	graph_queue.push(m_graph);

	for (int i = 0; i < iterations; i++) {
		for (int j = 0; j < pow(2, i); j++)
		{
			vertex_graph act = graph_queue.front();
			graph_queue.pop();
			vertex_graph v1, v2;

			graph_utils::normalized_cut(act, v1, v2);

			graph_queue.push(v1);
			graph_queue.push(v2);
		}
	}

	while (!graph_queue.empty())
	{
		auto graph = graph_queue.front();
		graph_queue.pop();
		glm::vec3 c = get_random_color();
		for (const auto& v : graph.nodes)
		{
			for (auto p : v.points)
			{
				//p->uv_stretch = v.second.getColor();
				p->uv_stretch = c;
			}
		}
	}
}

void spectral_cluster(std::vector<file_loader::vertex>& points, int iterations, float threshold) {
	auto supervoxels = supervoxel::supervoxelSegmentation(points, 0.5, 1);
	auto m_graph = vertex_graph(supervoxels);
	std::queue<vertex_graph> graph_queue;
	graph_queue.push(m_graph);

	std::vector<vertex_graph> final_subgraphs;

	while(!graph_queue.empty()) {

	//for (int i = 0; i < iterations; i++) {
	//	for (int j = 0; j < pow(2, i); j++)
	//	{
			vertex_graph act = graph_queue.front();
			graph_queue.pop();

			//graph_utils::normalized_cut(act, v1, v2);
#pragma region calc eigenvector
			const size_t num = act.nodes.size();

			Eigen::MatrixXf D(num, num);
			Eigen::MatrixXf W(num, num);

			//Create D diagonal matrix and W symmetric matrix and L laplacian matrix
			for (size_t i = 0; i < num; i++)
			{
				for (size_t j = 0; j < num; j++)
				{
					if (i == j)
					{
						D(i, j) = static_cast<float>(num);
						W(i, j) = 0.f;
					}
					else
					{
						D(i, j) = 0.f;
						W(i, j) = act.mat[i][j].get_weight();

					}
				}
			}
			//std::ofstream dout("D.txt");
			//std::ofstream wout("W.txt");
			//std::ofstream lout("L.txt");
			//dout << D << "\n";
			//wout << W << "\n";
			Eigen::MatrixXf L = D - W;
			//lout << L << "\n";

			//Solve generalized linear model
			Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXf> ges;
			ges.compute(L, D);

			//Get Fiedler vector (eigenvector of second smallest eigenvalue)
			int smallest = 0;
			int second_smallest = 1;
			int tmp;
			if (ges.eigenvalues()[smallest] > ges.eigenvalues()[second_smallest])
			{
				tmp = smallest;
				smallest = second_smallest;
				second_smallest = tmp;
			}

			for (int i = 2; i < ges.eigenvalues().size(); i++)
			{
				if (ges.eigenvalues()[i] < ges.eigenvalues()[smallest])
				{
					second_smallest = smallest;
					smallest = i;
				}
				else if (ges.eigenvalues()[i] < ges.eigenvalues()[second_smallest])
				{
					second_smallest = i;
				}
			}

			auto eigenvector = ges.eigenvectors().col(second_smallest);
#pragma endregion

#pragma check to threshold
			float average_cost = 0.f;
			int cut_count = 0;
			for (int i = 0; i < act.mat.size(); i++) {
				for (int j = 0; j < eigenvector.size(); j++)
				{
					if (i == j)
						continue;
					
					if (eigenvector[i] != eigenvector(j))
					{
						average_cost += act.mat[i][j].accumulated_w;
						cut_count++;
					}
				}
			}
			average_cost /= cut_count;

			if (average_cost < threshold) {
#pragma region cut if allowed
				std::vector<file_loader::vertex*> out_1;
				std::vector<file_loader::vertex*> out_2;
				for (int i = 0; i < act.nodes.size(); i++)
				{
					auto curr = act.nodes[i];
					for (auto v : curr.points)
					{
						if (eigenvector[i] < 0)
						{
							out_1.push_back(v);
						}
						else
						{
							out_2.push_back(v);
						}
					}
				}
				vertex_graph v1, v2;
				v1 = vertex_graph(supervoxel::supervoxelSegmentation(out_1, 0.5, 1));
				v2 = vertex_graph(supervoxel::supervoxelSegmentation(out_2, 0.5, 1));
#pragma endregion

				if (v1.nodes.size() >= 3 && v2.nodes.size() >= 3)
				{
					graph_queue.push(v1);
					graph_queue.push(v2);
				}
				else
				{
					final_subgraphs.push_back(act);
				}
			}
			else 
			{
				final_subgraphs.push_back(act);
			}
#pragma endregion
		}
	//}

	for(auto graph : final_subgraphs)
	{
		glm::vec3 c = get_random_color();
		for (const auto& v : graph.nodes)
		{
			for (auto p : v.points)
			{
				//p->uv_stretch = v.second.getColor();
				p->uv_stretch = c;
			}
		}
	}
}

glm::vec3 get_random_color() {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> hue_distribution(0.0f, 360.0f);
	const float h = hue_distribution(gen);
	const float s = 0.5f;
	const float l = 0.5f;
	return hsl_to_rgb(h, s, l);
}

glm::vec3 hsl_to_rgb(const float h, const float s, const float l) {
	const float c = (1.0f - fabs(2.0f * l - 1.0f)) * s;
	const float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
	const float m = l - c / 2.0f;

	glm::vec3 color;
	if (h < 60.0f) {
		color = glm::vec3(c, x, 0);
	}
	else if (h < 120.0f) {
		color = glm::vec3(x, c, 0);
	}
	else if (h < 180.0f) {
		color = glm::vec3(0, c, x);
	}
	else if (h < 240.0f) {
		color = glm::vec3(0, x, c);
	}
	else if (h < 300.0f) {
		color = glm::vec3(x, 0, c);
	}
	else {
		color = glm::vec3(c, 0, x);
	}

	return color + m;
}