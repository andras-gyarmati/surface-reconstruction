#pragma once
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include "glm/ext.hpp"
#include "file_loader.h"
#include <queue>
#include <limits>

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


class octree_vertex {
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

    file_loader::vertex* m_point = nullptr;
    node_state m_state = empty;

public:
    file_loader::vertex* m_top_left_front = nullptr;
    file_loader::vertex* m_bottom_right_back = nullptr;
    std::vector<octree_vertex*> m_children;

    octree_vertex() {
        m_point = nullptr;
        m_state = empty;
    }

    octree_vertex(file_loader::vertex& pos) {
        m_point = &pos;
        m_state = leaf;
    }

    octree_vertex(file_loader::vertex& tlf, file_loader::vertex& brb) {
        if (brb.position.x < tlf.position.x || brb.position.y < tlf.position.y || brb.position.z < tlf.position.z) {
            std::cout << "Boundary points are not valid" << std::endl;
            return;
        }

        m_point = nullptr;
        m_state = internal;
        m_top_left_front = &tlf;
        m_bottom_right_back = &brb;

        m_children.assign(8, nullptr);
        for (int i = top_left_front; i <= bottom_left_back; ++i)
            m_children[i] = new octree_vertex();
    }

    octree_vertex(const glm::vec3 tlf, const glm::vec3 brb) {
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

    void insert(file_loader::vertex& point_to_insert) {
        if (find(point_to_insert)) {
            return;
        }

        if (point_to_insert.position.x < m_top_left_front->position.x || point_to_insert.position.x > m_bottom_right_back->position.x ||
            point_to_insert.position.y < m_top_left_front->position.y || point_to_insert.position.y > m_bottom_right_back->position.y ||
            point_to_insert.position.z < m_top_left_front->position.z || point_to_insert.position.z > m_bottom_right_back->position.z) {
            return;
        }

        const glm::vec3 mid = (m_top_left_front->position + m_bottom_right_back->position) / 2.f;

        const int octant = get_octant(point_to_insert.position, mid);

        if (m_children[octant]->m_state == internal) {
            m_children[octant]->insert(point_to_insert);
            return;
        }
        if (m_children[octant]->m_state == empty) {
            delete m_children[octant];
            m_children[octant] = new octree_vertex(point_to_insert);
            return;
        }
        file_loader::vertex* already_stored_point = m_children[octant]->m_point;
        delete m_children[octant];
        m_children[octant] = nullptr;
        m_children[octant] = new octree_vertex(*m_top_left_front, *m_bottom_right_back);
        m_children[octant]->insert(*already_stored_point);
        m_children[octant]->insert(point_to_insert);
        m_children[octant]->m_point = nullptr;
        m_children[octant]->m_state = internal;
    }

    bool find(const file_loader::vertex& pos) const {
        if (pos.position.x < m_top_left_front->position.x || pos.position.x > m_bottom_right_back->position.x ||
            pos.position.y < m_top_left_front->position.y || pos.position.y > m_bottom_right_back->position.y ||
            pos.position.z < m_top_left_front->position.z || pos.position.z > m_bottom_right_back->position.z) {
            return false;
        }

        const glm::vec3 mid = (m_top_left_front->position + m_bottom_right_back->position) / 2.f;

        const int octant = get_octant(pos.position, mid);

        if (m_children[octant]->m_state == internal) {
            return m_children[octant]->find(pos);
        }
        if (m_children[octant]->m_state == empty) {
            return false;
        }
        return pos.position == m_children[octant]->m_point->position;
    }

    static int get_octant(const glm::vec3 point, const glm::vec3 mid) {
        return (point.x <= mid.x) ? ((point.y <= mid.y) ? ((point.z <= mid.z) ? top_left_front : top_left_bottom) : ((point.z <= mid.z) ? bottom_left_front : bottom_left_back))
            : ((point.y <= mid.y) ? ((point.z <= mid.z) ? top_right_front : top_right_bottom) : ((point.z <= mid.z) ? bottom_right_front : bottom_right_back));
    }

    struct Neighbor {
        file_loader::vertex* point;
        float distance;

        bool operator<(const Neighbor& other) const {
            return distance < other.distance;
        }
    };

    void knn_search(octree_vertex* node, const file_loader::vertex& target, int k, std::priority_queue<Neighbor>& nearest) {
        if (!node || node->m_state == octree_vertex::empty) return;

        if (node->m_state == octree_vertex::leaf && node->m_point) {
            float dist = glm::distance(node->m_point->position, target.position);
            if (nearest.size() < k) {
                nearest.push({ node->m_point, dist });
            }
            else if (dist < nearest.top().distance) {
                nearest.pop();
                nearest.push({ node->m_point, dist });
            }
            return;
        }

        int octant = octree_vertex::get_octant(target.position, (node->m_top_left_front->position + node->m_bottom_right_back->position) / 2.0f);
        knn_search(node->m_children[octant], target, k, nearest);

        for (int i = 0; i < 8; i++) {
            if (i != octant && node->m_children[i]) {
                float boundary_dist = glm::distance(target.position, (node->m_top_left_front->position + node->m_bottom_right_back->position) / 2.0f);
                if (nearest.size() < k || boundary_dist < nearest.top().distance) {
                    knn_search(node->m_children[i], target, k, nearest);
                }
            }
        }
    }

    std::vector<file_loader::vertex*> find_k_nearest(const file_loader::vertex& target, int k) {
        std::priority_queue<Neighbor> nearest;
        knn_search(this, target, k, nearest);

        std::vector<file_loader::vertex*> result;
        while (!nearest.empty()) {
            result.push_back(nearest.top().point);
            nearest.pop();
        }
        return result;
    }

    struct boundary {
        glm::vec3 m_top_left_front;
        glm::vec3 m_bottom_right_back;

        bool contains(const glm::vec3& point) const {
            return point.x >= m_top_left_front.x && point.x <= m_bottom_right_back.x &&
                point.y >= m_top_left_front.y && point.y <= m_bottom_right_back.y &&
                point.z >= m_top_left_front.z && point.z <= m_bottom_right_back.z;
        }

        bool contains(const file_loader::vertex& point) const {
            return point.position.x >= m_top_left_front.x && point.position.x <= m_bottom_right_back.x &&
                point.position.y >= m_top_left_front.y && point.position.y <= m_bottom_right_back.y &&
                point.position.z >= m_top_left_front.z && point.position.z <= m_bottom_right_back.z;
        }
    };

    static boundary calc_boundary(const std::vector<file_loader::vertex>& vertices) {
        glm::vec3 top_left_front = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 bottom_right_back = glm::vec3(std::numeric_limits<float>::min());
        for (const file_loader::vertex& v : vertices) {
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
        return boundary{ top_left_front, bottom_right_back };
    }
};