#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "file_loader.h"

class delaunay_3d {
public:
    struct face {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;

        bool operator==(const face& other) const {
            return (a == other.a || a == other.b || a == other.c) &&
                (b == other.a || b == other.b || b == other.c) &&
                (c == other.a || c == other.b || c == other.c);
        }
    };

    struct tetrahedron {
        glm::vec3 m_vertices[4]{};
        face m_faces[4]{};

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

        void init_faces() {
            m_faces[0] = {m_vertices[0], m_vertices[1], m_vertices[2]};
            m_faces[1] = {m_vertices[0], m_vertices[1], m_vertices[3]};
            m_faces[2] = {m_vertices[0], m_vertices[2], m_vertices[3]};
            m_faces[3] = {m_vertices[1], m_vertices[2], m_vertices[3]};
        }

        bool contains_face(const face& face) {
            for (const delaunay_3d::face& f : m_faces) {
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
            init_faces();
        }

        tetrahedron(const glm::vec3 a, const glm::vec3 b, const glm::vec3 c, const glm::vec3 d) {
            m_vertices[0] = a;
            m_vertices[1] = b;
            m_vertices[2] = c;
            m_vertices[3] = d;
            init_faces();
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

        static float length2(const glm::vec3 v) {
            return v.x * v.x + v.y * v.y + v.z * v.z;
        }

        glm::vec3 get_circumcenter() const {
            glm::vec3 center;

            //Create the rows of our "unrolled" 3x3 matrix
            const glm::vec3 row1 = m_vertices[1] - m_vertices[0];
            const float sq_len_1 = length2(row1);
            const glm::vec3 row2 = m_vertices[2] - m_vertices[0];
            const float sq_len_2 = length2(row2);
            const glm::vec3 row3 = m_vertices[3] - m_vertices[0];
            const float sq_len_3 = length2(row3);

            //Compute the determinant of said matrix
            const float determinant = row1.x * (row2.y * row3.z - row3.y * row2.z)
                - row2.x * (row1.y * row3.z - row3.y * row1.z)
                + row3.x * (row1.y * row2.z - row2.y * row1.z);

            // Compute the volume of the tetrahedron, and precompute a scalar quantity for re-use in the formula
            const float volume = determinant / 6.f;
            const float i12 = 1.f / (volume * 12.f);

            center.x = m_vertices[0].x + i12 * ((row2.y * row3.z - row3.y * row2.z) * sq_len_1 - (row1.y * row3.z - row3.y * row1.z) * sq_len_2 + (row1.y * row2.z - row2.y * row1.z) * sq_len_3);
            center.y = m_vertices[0].y + i12 * (-(row2.x * row3.z - row3.x * row2.z) * sq_len_1 + (row1.x * row3.z - row3.x * row1.z) * sq_len_2 - (row1.x * row2.z - row2.x * row1.z) * sq_len_3);
            center.z = m_vertices[0].z + i12 * ((row2.x * row3.y - row3.x * row2.y) * sq_len_1 - (row1.x * row3.y - row3.x * row1.y) * sq_len_2 + (row1.x * row2.y - row2.x * row1.y) * sq_len_3);
            return center;
        }

        bool is_point_inside_circumsphere(const glm::vec3 point) const {
            const glm::vec3 circumcenter = get_circumcenter();
            const float radius = glm::distance(circumcenter, m_vertices[0]);
            const float distance = glm::distance(point, circumcenter);
            return distance < radius;
        }

        bool is_face_shared_by_any_other_tetrahedra_in(std::vector<tetrahedron>& bad_tetrahedra, const face& face) const {
            for (tetrahedron& bad_tetrahedron : bad_tetrahedra) {
                if (&bad_tetrahedron != this && bad_tetrahedron.contains_face(face)) {
                    return true;
                }
            }
            return false;
        }

        bool contains_a_vertex_from_original_super_tetrahedron(const tetrahedron& root) const {
            const float epsilon = 0.001f;
            for (const glm::vec3& vertex : m_vertices) {
                if (glm::distance(vertex, root.m_vertices[0]) < epsilon ||
                    glm::distance(vertex, root.m_vertices[1]) < epsilon ||
                    glm::distance(vertex, root.m_vertices[2]) < epsilon ||
                    glm::distance(vertex, root.m_vertices[3]) < epsilon) {
                    return true;
                }
            }
            return false;
        }
    };

    std::vector<tetrahedron> m_tetrahedra;
    tetrahedron m_root;
    std::vector<tetrahedron> m_bad_tetrahedra;
    std::vector<face> m_poly_body;

    explicit delaunay_3d(const float side_length, glm::vec3 center = glm::vec3(0, 0, 0)) {
        m_root = tetrahedron(side_length, center);
        m_tetrahedra.push_back(m_root);
    }

    void cleanup_super_tetrahedron() {
        for (tetrahedron& tetrahedron : m_tetrahedra) {
            // done inserting points, now clean up
            if (tetrahedron.contains_a_vertex_from_original_super_tetrahedron(m_root)) {
                m_tetrahedra.erase(std::remove(m_tetrahedra.begin(), m_tetrahedra.end(), tetrahedron), m_tetrahedra.end());
            }
        }
    }

    bool get_side(const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3, const glm::vec3 point) const {
        const auto normal = glm::cross(v2 - v1, v3 - v1);
        const auto dot_p = glm::dot(normal, point - v1);
        return glm::sign(dot_p) > 0;
    }

    void insert_point(const file_loader::vertex& point) {
        if (!m_root.is_point_in_tetrahedron(point.position)) {
            return;
        }

        // add all the points one at a time to the m_tetrahedra
        m_bad_tetrahedra.clear();
        for (tetrahedron& tetrahedron : m_tetrahedra) {
            // first find all the _tetrahedra that are no longer valid due to the insertion
            if (tetrahedron.is_point_inside_circumsphere(point.position)) {
                m_bad_tetrahedra.push_back(tetrahedron);
            }
        }
        m_poly_body.clear();
        for (tetrahedron& bad_tetrahedron : m_bad_tetrahedra) {
            // find the boundary of the polygonal hole
            for (face& bad_face : bad_tetrahedron.m_faces) {
                if (!bad_tetrahedron.is_face_shared_by_any_other_tetrahedra_in(m_bad_tetrahedra, bad_face)) {
                    m_poly_body.push_back(bad_face);
                }
            }
        }
        for (tetrahedron& tetrahedron : m_bad_tetrahedra) {
            // remove them from the data structure
            m_tetrahedra.erase(std::remove(m_tetrahedra.begin(), m_tetrahedra.end(), tetrahedron), m_tetrahedra.end());
        }
        for (const face& face : m_poly_body) {
            // re-triangulate the polygonal hole
            tetrahedron new_tetrahedron;
            if (get_side(face.c, face.b, face.a, point.position)) {
                new_tetrahedron = tetrahedron(face.a, face.b, face.c, point.position);
            } else {
                new_tetrahedron = tetrahedron(face.c, face.b, face.a, point.position);
            }
            m_tetrahedra.push_back(new_tetrahedron);
        }
    }

    std::vector<tetrahedron> create_mesh(const std::vector<file_loader::vertex>& vertices) {
        for (const file_loader::vertex& point : vertices) {
            insert_point(point);
        }
        cleanup_super_tetrahedron();
        return m_tetrahedra;
    }

    delaunay_3d(void) = default;
};
