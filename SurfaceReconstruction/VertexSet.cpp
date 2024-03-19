#include "VertexSet.h"
#include <iostream>

VertexSet::VertexSet()
{

}

VertexSet::VertexSet(std::vector<file_loader::vertex>& points)
{
	this->points = points;
}

std::vector<file_loader::vertex>& VertexSet::get_points()
{
	return points;
}

const std::vector<file_loader::vertex>& VertexSet::get_points() const
{
	return points;
}

std::vector<std::vector<int>> VertexSet::get_grouped_points()
{
	return groups;
}

std::vector<int> VertexSet::get_non_grouped()
{
	std::vector<int> result;
	for (size_t i = 0; i < points.size(); i++)
	{
		if (!points[i].is_grouped) result.push_back(i);
	}

	return result;
}

std::vector<file_loader::vertex>& VertexSet::get_points_to_render()
{
	return points;
		
	//static std::vector<file_loader::vertex> result;
	//result.clear();

	//if (group_count() == 0)
	//{
	//	result = points;
	//}
	//else
	//{
	//	int i = 0;
	//	for (std::vector<int> group : groups)
	//	{
	//		if (static_cast<bool>(show_group[i]))
	//		{
	//			for (int j : group)
	//			{
	//				result.push_back(points[j]);
	//			}
	//		}
	//		i++;
	//	}

	//	for (size_t i = 0; i < points.size(); i++)
	//	{
	//		if (!points[i].is_grouped)
	//		{
	//			result.push_back(points[i]);
	//		}
	//	}
	//}

	//return result;
}

void VertexSet::create_group(std::vector<int> points_to_group)
{
	std::vector<int> group;
	for each (int idx in points_to_group)
	{
		points[idx].is_grouped = true;
		group.push_back(idx);
	}
	groups.push_back(group);
	show_group.push_back(static_cast<char>(true));
}

size_t VertexSet::size() const
{
	return points.size();
}

size_t VertexSet::group_count() const
{
	return groups.size();
}

file_loader::vertex VertexSet::operator[](int i) const
{
	return points[i];
}

file_loader::vertex& VertexSet::operator[](int i)
{
	return points[i];
}

std::vector<char>& VertexSet::get_show_groups()
{
	return show_group;
}