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
        tetrahedron* children[4]{};

        tetrahedron(void) = default;

        explicit tetrahedron(const float side_length) {
            m_vertices[0] = glm::vec3(0, 0, side_length * sqrt(2.0f / 3.0f));
            m_vertices[1] = glm::vec3(0, 2.0f * side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            m_vertices[2] = glm::vec3((-sqrt(3.0f) / sqrt(6.0f)) * side_length, -side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            m_vertices[3] = glm::vec3((sqrt(3.0f) / sqrt(6.0f)) * side_length, -side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            for (auto& child : children) {
                child = nullptr;
            }
        }

        tetrahedron(const glm::vec3 a, const glm::vec3 b, const glm::vec3 c, const glm::vec3 d) {
            m_vertices[0] = a;
            m_vertices[1] = b;
            m_vertices[2] = c;
            m_vertices[3] = d;
            for (auto& child : children) {
                child = nullptr;
            }
        }

        bool is_same_side(const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3, const glm::vec3 v4, const glm::vec3 point) const {
            const auto normal = glm::cross(v2 - v1, v3 - v1);
            const auto dot_v4 = glm::dot(normal, v4 - v1);
            const auto dot_p = glm::dot(normal, point - v1);
            return glm::sign(dot_v4) == glm::sign(dot_p);
        }

        bool is_point_in_tetrahedron(const glm::vec3 point) const {
            return is_same_side(m_vertices[0], m_vertices[1], m_vertices[2], m_vertices[3], point) &&
                is_same_side(m_vertices[1], m_vertices[2], m_vertices[3], m_vertices[0], point) &&
                is_same_side(m_vertices[2], m_vertices[3], m_vertices[0], m_vertices[1], point) &&
                is_same_side(m_vertices[3], m_vertices[0], m_vertices[1], m_vertices[2], point);
        }

        void insert(const glm::vec3& point) {
            if (is_point_in_tetrahedron(point)) {
                if (children[0] == nullptr) {
                    children[0] = new tetrahedron(m_vertices[0], m_vertices[1], m_vertices[2], point);
                    children[1] = new tetrahedron(m_vertices[1], m_vertices[0], m_vertices[3], point);
                    children[2] = new tetrahedron(m_vertices[2], m_vertices[1], m_vertices[3], point);
                    children[3] = new tetrahedron(m_vertices[0], m_vertices[2], m_vertices[3], point);
                }
            }
        }
    };

    std::vector<tetrahedron> m_tetrahedra;
    tetrahedron m_root;

    explicit delaunay(const float side_length) {
        m_root = tetrahedron(side_length);
        m_tetrahedra.push_back(m_root);
    }

    void insert(const glm::vec3& point) {
        m_root.insert(point);
    }

    delaunay(void) = default;
};
