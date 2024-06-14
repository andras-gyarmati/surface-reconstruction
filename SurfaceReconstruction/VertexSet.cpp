#include "VertexSet.h"
#include <iostream>

VertexSet::VertexSet()
{

}

VertexSet::VertexSet(std::vector<file_loader::vertex>& points) : points(points), show_group({ static_cast<char>(false) }) { }

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

std::vector<file_loader::vertex>& VertexSet::get_points_to_render() const
{
	static std::vector<file_loader::vertex> result;
	result.clear();

	int i = 0;
	file_loader::vertex prev;
	for (const file_loader::vertex& v : points)
	{
		if (v.group_id != 0)
		{
			prev = v;
			break;
		}
	}

	for (const file_loader::vertex& v : points)
	{
		if (static_cast<bool>(show_group[v.group_id]))
		{
			result.push_back(v);
			prev = v;
		}
		else
		{
			result.push_back(prev);
		}
	}

	return result;
}

void VertexSet::create_group(std::vector<int> points_to_group)
{
	static int ID = 1;

	std::vector<int> group;
	for (int idx : points_to_group)
	{
		if (points[idx].group_id == 0)
		{
			points[idx].group_id = ID;
			points[idx].is_grouped = true;
			group.push_back(idx);
		}
	}

	groups.push_back(group);
	show_group.push_back(static_cast<char>(true));
	ID++;
}

size_t VertexSet::size() const
{
	return points.size();
}

size_t VertexSet::group_count() const
{
	return show_group.size();
}

std::vector<std::vector<int>> VertexSet::get_shown_groups()
{
	static std::vector<std::vector<int>> result;
	result.clear();
	
	for (int i = 1; i < show_group.size(); i++)
	{
		if (static_cast<bool>(show_group[i]))
		{
			result.push_back(groups[i - 1]);
		}
	}
	
	return result;
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
