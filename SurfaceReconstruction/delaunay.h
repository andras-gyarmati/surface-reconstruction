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

        // bool operator==(const face& other) const {
        //     auto this_face_points = std::array<glm::vec3, 3>{a, b, c};
        //     auto other_face_points = std::array<glm::vec3, 3>{other.a, other.b, other.c};
        //
        //     auto point_sum = [](const glm::vec3& point) {
        //         return point.x + point.y + point.z;
        //     };
        //     std::sort(this_face_points.begin(),
        //               this_face_points.end(),
        //               [&point_sum](const glm::vec3& lhs, const glm::vec3& rhs) {
        //                   return point_sum(lhs) < point_sum(rhs);
        //               });
        //     std::sort(other_face_points.begin(),
        //               other_face_points.end(),
        //               [&point_sum](const glm::vec3& lhs, const glm::vec3& rhs) {
        //                   return point_sum(lhs) < point_sum(rhs);
        //               });
        //
        //     return this_face_points == other_face_points;
        // }

        bool operator==(const face& other) const {
            return (a == other.a || a == other.b || a == other.c) &&
                (b == other.a || b == other.b || b == other.c) &&
                (c == other.a || c == other.b || c == other.c);
        }
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

        bool operator!=(const tetrahedron& tetrahedron) const {
            return !(*this == tetrahedron);
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

        bool contains_face(const face& face) {
            for (const auto& f : m_faces) {
                if (f == face) {
                    return true;
                }
            }
            return false;
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

        glm::vec3 get_circumcenter() const {
            // Use coordinates relative to point 'a' of the tetrahedron.

            const glm::vec3 ba = m_vertices[1] - m_vertices[0];
            const glm::vec3 ca = m_vertices[2] - m_vertices[0];
            const glm::vec3 da = m_vertices[3] - m_vertices[0];

            // Squares of lengths of the edges incident to 'a'.
            const float len_ba = ba.x * ba.x + ba.y * ba.y + ba.z * ba.z;
            const float len_ca = ca.x * ca.x + ca.y * ca.y + ca.z * ca.z;
            const float len_da = da.x * da.x + da.y * da.y + da.z * da.z;

            // c cross d
            const float cross_cd_x = ca.y * da.z - da.y * ca.z;
            const float cross_cd_y = ca.z * da.x - da.z * ca.x;
            const float cross_cd_z = ca.x * da.y - da.x * ca.y;

            // d cross b
            const float cross_db_x = da.y * ba.z - ba.y * da.z;
            const float cross_db_y = da.z * ba.x - ba.z * da.x;
            const float cross_db_z = da.x * ba.y - ba.x * da.y;

            // b cross c
            const float cross_bc_x = ba.y * ca.z - ca.y * ba.z;
            const float cross_bc_y = ba.z * ca.x - ca.z * ba.x;
            const float cross_bc_z = ba.x * ca.y - ca.x * ba.y;

            // Calculate the denominator of the formula.
            const float denominator = 0.5f / (ba.x * cross_cd_x + ba.y * cross_cd_y + ba.z * cross_cd_z);

            // Calculate offset (from 'a') of circumcenter.
            const float circ_x = (len_ba * cross_cd_x + len_ca * cross_db_x + len_da * cross_bc_x) * denominator;
            const float circ_y = (len_ba * cross_cd_y + len_ca * cross_db_y + len_da * cross_bc_y) * denominator;
            const float circ_z = (len_ba * cross_cd_z + len_ca * cross_db_z + len_da * cross_bc_z) * denominator;

            const auto circumcenter = glm::vec3(circ_x, circ_y, circ_z);
            return circumcenter;
        }

        bool is_point_inside_circumsphere(const glm::vec3 point) const {
            const auto circumcenter = get_circumcenter();
            const auto radius = glm::distance(circumcenter, m_vertices[0]);
            const auto distance = glm::distance(point, circumcenter);
            return distance < radius;
        }

        bool is_face_shared_by_any_other_tetrahedra_in(const std::vector<tetrahedron>& bad_tetrahedra, const face& face) const {
            for (auto tetrahedron : bad_tetrahedra) {
                if (tetrahedron != *this && tetrahedron.contains_face(face)) {
                    return true;
                }
            }
            return false;
        }

        bool contains_a_vertex_from_original_super_tetrahedron(const tetrahedron& root) const {
            for (auto vertex : m_vertices) {
                if (vertex == root.m_vertices[0] || vertex == root.m_vertices[1] || vertex == root.m_vertices[2] || vertex == root.m_vertices[3]) {
                    return true;
                }
            }
            return false;
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

    std::vector<tetrahedron> create_mesh(std::vector<file_loader::vertex> vertices) {
        for (auto point : vertices) {
            // add all the points one at a time to the m_tetrahedra
            std::vector<tetrahedron> bad_tetrahedra;
            for (auto tetrahedron : m_tetrahedra) {
                // first find all the _tetrahedra that are no longer valid due to the insertion
                if (tetrahedron.is_point_inside_circumsphere(point.position)) {
                    bad_tetrahedra.push_back(tetrahedron);
                }
            }
            std::vector<face> poly_body;
            for (auto tetrahedron : bad_tetrahedra) {
                // find the boundary of the polygonal hole
                for (auto face : tetrahedron.m_faces) {
                    if (!tetrahedron.is_face_shared_by_any_other_tetrahedra_in(bad_tetrahedra, face)) {
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
            if (tetrahedron.contains_a_vertex_from_original_super_tetrahedron(m_root)) {
                m_tetrahedra.erase(std::remove(m_tetrahedra.begin(), m_tetrahedra.end(), tetrahedron), m_tetrahedra.end());
            }
        }
        return m_tetrahedra;
    }

    delaunay(void) = default;
};
