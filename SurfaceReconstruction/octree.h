#pragma once
#include <vector>
#include "glm/ext.hpp"
// #include "glm/gtx/string_cast.hpp"

enum octant {
    top_left_front,
    top_right_front,
    bottom_right_front,
    bottom_left_front,
    top_left_bottom,
    top_right_bottom,
    bottom_right_back,
    bottom_left_back
};

class octree {
    glm::vec3* m_point;
    glm::vec3* m_top_left_front;
    glm::vec3* m_bottom_right_back;
    std::vector<octree*> m_children;
    const float epsilon = 1e-6f;

public:
    octree();
    octree(glm::vec3 p);
    ~octree();
    octree(const glm::vec3 tlf, const glm::vec3 brb);
    void insert(const glm::vec3 pos);
    bool find(glm::vec3 pos) const;
};
