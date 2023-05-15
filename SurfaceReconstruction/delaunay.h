#pragma once
#include <vector>
#include <glm/glm.hpp>

class delaunay {
public:
    struct face {
        glm::vec3 vertices[3];
    };

    struct tetrahedron {
        glm::vec3 m_vertices[4];
        // tetrahedron* children[4];

        tetrahedron(const float side_length) {
            m_vertices[0] = glm::vec3(0, 0, side_length * sqrt(2.0f / 3.0f));
            m_vertices[1] = glm::vec3(0, 2.0f * side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            m_vertices[2] = glm::vec3((-sqrt(3.0f) / sqrt(6.0f)) * side_length, -side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            m_vertices[3] = glm::vec3((sqrt(3.0f) / sqrt(6.0f)) * side_length, -side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            // for (auto& c : children) {
            //     c = nullptr;
            // }
        }
    };

    std::vector<tetrahedron> m_tetrahedra;

    delaunay(const float side_length) {
        m_tetrahedra.push_back(tetrahedron(side_length));
    }

    delaunay(void) = default;
};
