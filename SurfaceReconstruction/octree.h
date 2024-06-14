#pragma once
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include "glm/ext.hpp"

class octree {
    enum octant {
        top_left_front = 0,
        top_right_front = 1,
        bottom_right_front = 2,
        bottom_left_front = 3,
        top_left_bottom = 4,
        top_right_bottom = 5,
        bottom_right_back = 6,
        bottom_left_back = 7
    };

    enum node_state {
        empty = 0,
        internal = 1,
        leaf = 2
    };

    glm::vec3* m_point = nullptr;
    node_state m_state = empty;

public:
    glm::vec3* m_top_left_front = nullptr;
    glm::vec3* m_bottom_right_back = nullptr;
    std::vector<octree*> m_children;

    octree() {
        m_point = nullptr;
        m_state = empty;
    }

    octree(const glm::vec3 pos) {
        m_point = new glm::vec3(pos);
        m_state = leaf;
    }

    octree(const glm::vec3 tlf, const glm::vec3 brb) {
        if (brb.x < tlf.x || brb.y < tlf.y || brb.z < tlf.z) {
            std::cout << "Boundary points are not valid" << std::endl;
            return;
        }

        m_point = nullptr;
        m_state = internal;
        m_top_left_front = new glm::vec3(tlf);
        m_bottom_right_back = new glm::vec3(brb);

        m_children.assign(8, nullptr);
        for (int i = top_left_front; i <= bottom_left_back; ++i)
            m_children[i] = new octree();
    }

    void insert(const glm::vec3 point_to_insert) {
        if (find(point_to_insert)) {
            //std::cout << "Point already exists in the tree" << " pos: " << glm::to_string(point_to_insert) << std::endl;
            return;
        }

        if (point_to_insert.x < m_top_left_front->x || point_to_insert.x > m_bottom_right_back->x || point_to_insert.y <
            m_top_left_front->y ||
            point_to_insert.y > m_bottom_right_back->y || point_to_insert.z < m_top_left_front->z || point_to_insert.z >
            m_bottom_right_back->z) {
            //std::cout << "Point is out of bounds." << " pos: " << glm::to_string(point_to_insert) <<
            //    " m_top_left_front: "
            //    << glm::to_string(*m_top_left_front) << " m_bottom_right_back: " << glm::to_string(*m_bottom_right_back)
            //    << std::endl;
            return;
        }

        const glm::vec3 mid = (*m_top_left_front + *m_bottom_right_back) / 2.f;

        const int octant = get_octant(point_to_insert, mid);

        if (m_children[octant]->m_state == internal) {
            m_children[octant]->insert(point_to_insert);
            return;
        }
        if (m_children[octant]->m_state == empty) {
            delete m_children[octant];
            m_children[octant] = new octree(point_to_insert);
            return;
        }
        const glm::vec3 already_stored_point = *m_children[octant]->m_point;
        delete m_children[octant];
        m_children[octant] = nullptr;
        if (octant == top_left_front) {
            m_children[octant] = new octree(glm::vec3(m_top_left_front->x, m_top_left_front->y, m_top_left_front->z), glm::vec3(mid.x, mid.y, mid.z));
        } else if (octant == top_right_front) {
            m_children[octant] = new octree(glm::vec3(mid.x, m_top_left_front->y, m_top_left_front->z), glm::vec3(m_bottom_right_back->x, mid.y, mid.z));
        } else if (octant == bottom_right_front) {
            m_children[octant] = new octree(glm::vec3(mid.x, mid.y, m_top_left_front->z), glm::vec3(m_bottom_right_back->x, m_bottom_right_back->y, mid.z));
        } else if (octant == bottom_left_front) {
            m_children[octant] = new octree(glm::vec3(m_top_left_front->x, mid.y, m_top_left_front->z), glm::vec3(mid.x, m_bottom_right_back->y, mid.z));
        } else if (octant == top_left_bottom) {
            m_children[octant] = new octree(glm::vec3(m_top_left_front->x, m_top_left_front->y, mid.z), glm::vec3(mid.x, mid.y, m_bottom_right_back->z));
        } else if (octant == top_right_bottom) {
            m_children[octant] = new octree(glm::vec3(mid.x, m_top_left_front->y, mid.z), glm::vec3(m_bottom_right_back->x, mid.y, m_bottom_right_back->z));
        } else if (octant == bottom_right_back) {
            m_children[octant] = new octree(glm::vec3(mid.x, mid.y, mid.z), glm::vec3(m_bottom_right_back->x, m_bottom_right_back->y, m_bottom_right_back->z));
        } else if (octant == bottom_left_back) {
            m_children[octant] = new octree(glm::vec3(m_top_left_front->x, mid.y, mid.z), glm::vec3(mid.x, m_bottom_right_back->y, m_bottom_right_back->z));
        }
        m_children[octant]->insert(already_stored_point);
        m_children[octant]->insert(point_to_insert);
        m_children[octant]->m_point = nullptr;
        m_children[octant]->m_state = internal;
    }

    bool find(const glm::vec3 pos) const {
        if (pos.x < m_top_left_front->x || pos.x > m_bottom_right_back->x || pos.y < m_top_left_front->y ||
            pos.y > m_bottom_right_back->y || pos.z < m_top_left_front->z || pos.z > m_bottom_right_back->z) {
            return false;
        }

        const glm::vec3 mid = (*m_top_left_front + *m_bottom_right_back) / 2.f;

        const int octant = get_octant(pos, mid);

        if (m_children[octant]->m_state == internal) {
            return m_children[octant]->find(pos);
        }
        if (m_children[octant]->m_state == empty) {
            return false;
        }
        return pos == *m_children[octant]->m_point;
    }

    static int get_octant(const glm::vec3 point, const glm::vec3 mid) {
        int octant;
        if (point.x <= mid.x) {
            if (point.y <= mid.y) {
                if (point.z <= mid.z)
                    octant = top_left_front;
                else
                    octant = top_left_bottom;
            } else {
                if (point.z <= mid.z)
                    octant = bottom_left_front;
                else
                    octant = bottom_left_back;
            }
        } else {
            if (point.y <= mid.y) {
                if (point.z <= mid.z)
                    octant = top_right_front;
                else
                    octant = top_right_bottom;
            } else {
                if (point.z <= mid.z)
                    octant = bottom_right_front;
                else
                    octant = bottom_right_back;
            }
        }
        return octant;
    }

    struct boundary {
        glm::vec3 m_top_left_front;
        glm::vec3 m_bottom_right_back;

        bool contains(const glm::vec3& point) const {
            return point.x >= m_top_left_front.x && point.x <= m_bottom_right_back.x &&
                point.y >= m_top_left_front.y && point.y <= m_bottom_right_back.y &&
                point.z >= m_top_left_front.z && point.z <= m_bottom_right_back.z;
        }
    };

    static boundary calc_boundary(const std::vector<file_loader::vertex>& vertices) {
        glm::vec3 top_left_front = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 bottom_right_back = glm::vec3(std::numeric_limits<float>::min());
        for (const file_loader::vertex& v /*[position, color, ransac, normal, uv_stretch, bfs_col, is_grouped, group_id]*/ : vertices) {
            if (v.position.x < top_left_front.x) {
                top_left_front.x = v.position.x;
            }
            if (v.position.y < top_left_front.y) {
                top_left_front.y = v.position.y;
            }
            if (v.position.z < top_left_front.z) {
                top_left_front.z = v.position.z;
            }
            if (v.position.x > bottom_right_back.x) {
                bottom_right_back.x = v.position.x;
            }
            if (v.position.y > bottom_right_back.y) {
                bottom_right_back.y = v.position.y;
            }
            if (v.position.z > bottom_right_back.z) {
                bottom_right_back.z = v.position.z;
            }
        }
        return boundary{top_left_front, bottom_right_back};
    }
};
