#pragma once
#include "file_loader.h"

class VertexSet
{
private:
	std::vector<file_loader::vertex> points;
	std::vector<std::vector<int>> groups;
	std::vector<char> show_group;

public:
	VertexSet();
	VertexSet(std::vector<file_loader::vertex>& vertices);
	~VertexSet() {};

	std::vector<file_loader::vertex>& get_points();
	const std::vector<file_loader::vertex>& get_points() const;
	std::vector<std::vector<int>> get_grouped_points();
	std::vector<int> get_non_grouped();
	std::vector<file_loader::vertex>& get_points_to_render();
	std::vector<char>& get_show_groups();

	void create_group(std::vector<int> points_to_group);

	size_t size() const;
	size_t group_count() const;
	
	file_loader::vertex operator[](int i) const;
	file_loader::vertex& operator[](int i);
};