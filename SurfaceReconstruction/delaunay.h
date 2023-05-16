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
        tetrahedron* m_children[4]{};

        tetrahedron(void) = default;

        explicit tetrahedron(const float side_length, glm::vec3 center = glm::vec3(0, 0, 0)) {
            m_vertices[0] = center + glm::vec3(0, 0, side_length * sqrt(2.0f / 3.0f));
            m_vertices[1] = center + glm::vec3(0, 2.0f * side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            m_vertices[2] = center + glm::vec3((-sqrt(3.0f) / sqrt(6.0f)) * side_length, -side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            m_vertices[3] = center + glm::vec3((sqrt(3.0f) / sqrt(6.0f)) * side_length, -side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            for (auto& child : m_children) {
                child = nullptr;
            }
        }

        tetrahedron(const glm::vec3 a, const glm::vec3 b, const glm::vec3 c, const glm::vec3 d) {
            m_vertices[0] = a;
            m_vertices[1] = b;
            m_vertices[2] = c;
            m_vertices[3] = d;
            for (auto& child : m_children) {
                child = nullptr;
            }
        }

        bool is_point_in_tetrahedron(const glm::vec3& point) const {
            const glm::vec3 v0 = m_vertices[1] - m_vertices[0];
            const glm::vec3 v1 = m_vertices[2] - m_vertices[0];
            const glm::vec3 v2 = m_vertices[3] - m_vertices[0];
            const glm::vec3 v3 = point - m_vertices[0];

            const float d0 = glm::dot(v0, glm::cross(v1, v2));
            const float d1 = glm::dot(v3, glm::cross(v1, v2));
            const float d2 = glm::dot(v0, glm::cross(v3, v2));
            const float d3 = glm::dot(v0, glm::cross(v1, v3));
            const float d4 = glm::dot(v0, glm::cross(v1, v2));

            return (std::signbit(d0) == std::signbit(d1)) &&
                (std::signbit(d0) == std::signbit(d2)) &&
                (std::signbit(d0) == std::signbit(d3)) &&
                (std::signbit(d0) == std::signbit(d4));
        }

        void insert(const glm::vec3& point) {
            if (is_point_in_tetrahedron(point)) {
                if (m_children[0] == nullptr) {
                    m_children[0] = new tetrahedron(m_vertices[0], m_vertices[1], m_vertices[2], point);
                    m_children[1] = new tetrahedron(m_vertices[1], m_vertices[0], m_vertices[3], point);
                    m_children[2] = new tetrahedron(m_vertices[2], m_vertices[1], m_vertices[3], point);
                    m_children[3] = new tetrahedron(m_vertices[0], m_vertices[2], m_vertices[3], point);
                } else {
                    m_children[0]->insert(point);
                    m_children[1]->insert(point);
                    m_children[2]->insert(point);
                    m_children[3]->insert(point);
                }
            }
        }
    };

    std::vector<tetrahedron> m_tetrahedra;
    tetrahedron m_root;

    explicit delaunay(const float side_length, glm::vec3 center = glm::vec3(0, 0, 0)) {
        m_root = tetrahedron(side_length, center);
        m_tetrahedra.push_back(m_root);
    }

    void insert(const glm::vec3& point) {
        m_root.insert(point);
    }

    delaunay(void) = default;
};
