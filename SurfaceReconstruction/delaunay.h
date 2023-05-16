#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "file_loader.h"

class delaunay {
public:
    struct face {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
    };

    struct edge {
        glm::vec3 a;
        glm::vec3 b;
    };

    struct tetrahedron {
        glm::vec3 m_vertices[4]{};
        face m_faces[4]{};
        edge m_edges[6]{};
        tetrahedron* m_children[4]{};

        tetrahedron(void) = default;

        bool operator==(const tetrahedron& other) const {
            return (m_vertices[0] == other.m_vertices[0]) &&
                (m_vertices[1] == other.m_vertices[1]) &&
                (m_vertices[2] == other.m_vertices[2]) &&
                (m_vertices[3] == other.m_vertices[3]);
        }

        void init_edges() {
            m_edges[0] = {m_vertices[0], m_vertices[1]};
            m_edges[1] = {m_vertices[0], m_vertices[2]};
            m_edges[2] = {m_vertices[0], m_vertices[3]};
            m_edges[3] = {m_vertices[1], m_vertices[2]};
            m_edges[4] = {m_vertices[1], m_vertices[3]};
            m_edges[5] = {m_vertices[2], m_vertices[3]};
        }

        void init_faces() {
            m_faces[0] = {m_vertices[0], m_vertices[1], m_vertices[2]};
            m_faces[1] = {m_vertices[0], m_vertices[1], m_vertices[3]};
            m_faces[2] = {m_vertices[0], m_vertices[2], m_vertices[3]};
            m_faces[3] = {m_vertices[1], m_vertices[2], m_vertices[3]};
        }

        explicit tetrahedron(const float side_length, glm::vec3 center = glm::vec3(0, 0, 0)) {
            m_vertices[0] = center + glm::vec3(0, 0, side_length * sqrt(2.0f / 3.0f));
            m_vertices[1] = center + glm::vec3(0, 2.0f * side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            m_vertices[2] = center + glm::vec3((-sqrt(3.0f) / sqrt(6.0f)) * side_length, -side_length / sqrt(6.0f), -side_length / sqrt(2.0f));
            m_vertices[3] = center + glm::vec3((sqrt(3.0f) / sqrt(6.0f)) * side_length, -side_length / sqrt(6.0f), -side_length / sqrt(2.0f));

            init_edges();

            init_faces();

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

    bool point_is_inside_circumsphere_of_tetrahedron(glm::vec3 point, delaunay::tetrahedron tetrahedron) {
        return false;
    }

    bool is_shared_by_any_other_tetrahedra_in(const std::vector<delaunay::tetrahedron>& bad_tetrahedra, const delaunay::face& face) {
        return false;
    }

    bool tetrahedron_contains_a_vertex_from_original_super_tetrahedron(const delaunay::tetrahedron& tetrahedron) {
        return false;
    }

    std::vector<tetrahedron> create_mesh(std::vector<file_loader::vertex> vertices) {
        for (auto point : vertices) {
            // add all the points one at a time to the m_tetrahedra
            std::vector<tetrahedron> bad_tetrahedra;
            for (auto tetrahedron : m_tetrahedra) {
                // first find all the _tetrahedra that are no longer valid due to the insertion
                if (point_is_inside_circumsphere_of_tetrahedron(point.position, tetrahedron)) {
                    bad_tetrahedra.push_back(tetrahedron);
                }
            }
            std::vector<face> poly_body;
            for (auto tetrahedron : bad_tetrahedra) {
                // find the boundary of the polygonal hole
                for (auto face : tetrahedron.m_faces) {
                    if (!is_shared_by_any_other_tetrahedra_in(bad_tetrahedra, face)) {
                        poly_body.push_back(face);
                    }
                }
            }
            for (auto tetrahedron : bad_tetrahedra) {
                // remove them from the data structure
                m_tetrahedra.erase(std::remove(m_tetrahedra.begin(), m_tetrahedra.end(), tetrahedron), m_tetrahedra.end());
            }
            for (auto face : poly_body) {
                // re-triangulate the polygonal hole
                auto new_tetrahedron = tetrahedron(face.a, face.b, face.c, point.position);
                m_tetrahedra.push_back(new_tetrahedron);
            }
        }
        for (auto tetrahedron : m_tetrahedra) {
            // done inserting points, now clean up
            if (tetrahedron_contains_a_vertex_from_original_super_tetrahedron(tetrahedron)) {
                m_tetrahedra.erase(std::remove(m_tetrahedra.begin(), m_tetrahedra.end(), tetrahedron), m_tetrahedra.end());
            }
        }
        return m_tetrahedra;
    }

    delaunay(void) = default;
};
