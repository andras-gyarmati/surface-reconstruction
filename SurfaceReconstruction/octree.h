#pragma once
#include <iostream>
#include <vector>
#include <glm/vec3.hpp>
#include "glm/ext.hpp"

class octree {
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

    struct point {
        glm::vec3 pos;
        int index;
    };

    point* m_point;
    glm::vec3* m_top_left_front;
    glm::vec3* m_bottom_right_back;
    std::vector<octree*> m_children;

public:
    octree() {
        m_point = new point{glm::vec3(-1, -1, -1), -1};
        m_top_left_front = new glm::vec3(0, 0, 0);
        m_bottom_right_back = new glm::vec3(0, 0, 0);
    }

    octree(glm::vec3 pos, const int index) {
        m_point = new point{glm::vec3(pos), index};
        m_top_left_front = new glm::vec3(0, 0, 0);
        m_bottom_right_back = new glm::vec3(0, 0, 0);
    }

    ~octree() {
        delete m_point;
        delete m_top_left_front;
        delete m_bottom_right_back;
        for (const auto child : m_children) {
            delete child;
        }
    }

    octree(const glm::vec3 tlf, const glm::vec3 brb) {
        if (brb.x < tlf.x || brb.y < tlf.y || brb.z < tlf.z) {
            std::cout << "boundary points are not valid" << std::endl;
            return;
        }

        m_point = nullptr;
        m_top_left_front = new glm::vec3(tlf);
        m_bottom_right_back = new glm::vec3(brb);

        m_children.assign(8, nullptr);
        for (int i = top_left_front; i <= bottom_left_back; ++i)
            m_children[i] = new octree();
    }

    void insert(glm::vec3 pos, const int index) {
        if (find(pos)) {
            std::cout << "Point already exists in the tree" << " pos: " << glm::to_string(pos) << std::endl;
            return;
        }

        if (pos.x < m_top_left_front->x || pos.x > m_bottom_right_back->x ||
            pos.y < m_top_left_front->y || pos.y > m_bottom_right_back->y ||
            pos.z < m_top_left_front->z || pos.z > m_bottom_right_back->z) {
            std::cout << "Point is out of bounds."
                << " pos: " << glm::to_string(pos)
                << " m_top_left_front: " << glm::to_string(*m_top_left_front)
                << " m_bottom_right_back: " << glm::to_string(*m_bottom_right_back) << std::endl;
            return;
        }

        const glm::vec3 mid = (*m_top_left_front + *m_bottom_right_back) / 2.f;

        const int octant = get_octant(pos, mid);
        if (m_children[octant]->m_point == nullptr) {
            m_children[octant]->insert(pos, index);
            return;
        }
        else if ((*m_children[octant]->m_point).pos == glm::vec3(-1, -1, -1)) {
            delete m_children[octant];
            m_children[octant] = new octree(pos, index);
            return;
        }
        else {
            const point p = *m_children[octant]->m_point;
            delete m_children[octant];
            m_children[octant] = nullptr;
            if (octant == top_left_front) {
                m_children[octant] = new octree(*m_top_left_front, mid);
            }
            else if (octant == top_right_front) {
                m_children[octant] = new octree(glm::vec3(mid.x, m_top_left_front->y, m_top_left_front->z), glm::vec3(m_bottom_right_back->x, mid.y, mid.z));
            }
            else if (octant == bottom_right_front) {
                m_children[octant] = new octree(glm::vec3(mid.x, mid.y, m_top_left_front->z), glm::vec3(m_bottom_right_back->x, m_bottom_right_back->y, mid.z));
            }
            else if (octant == bottom_left_front) {
                m_children[octant] = new octree(glm::vec3(m_top_left_front->x, mid.y, m_top_left_front->z), glm::vec3(mid.x, m_bottom_right_back->y, mid.z));
            }
            else if (octant == top_left_bottom) {
                m_children[octant] = new octree(glm::vec3(m_top_left_front->x, m_top_left_front->y, mid.z), glm::vec3(mid.x, mid.y, m_bottom_right_back->z));
            }
            else if (octant == top_right_bottom) {
                m_children[octant] = new octree(glm::vec3(mid.x, m_top_left_front->y, mid.z), glm::vec3(m_bottom_right_back->x, mid.y, m_bottom_right_back->z));
            }
            else if (octant == bottom_right_back) {
                m_children[octant] = new octree(glm::vec3(mid.x, mid.y, mid.z), *m_bottom_right_back);
            }
            else if (octant == bottom_left_back) {
                m_children[octant] = new octree(glm::vec3(m_top_left_front->x, mid.y, mid.z), glm::vec3(mid.x, m_bottom_right_back->y, m_bottom_right_back->z));
            }
            m_children[octant]->insert(p.pos, p.index);
            m_children[octant]->insert(pos, index);
        }
    }

    static int get_octant(const glm::vec3 pos, const glm::vec3 mid) {
        int octant;
        if (pos.x < mid.x) {
            if (pos.y < mid.y) {
                if (pos.z < mid.z)
                    octant = top_left_front;
                else
                    octant = top_left_bottom;
            }
            else {
                if (pos.z < mid.z)
                    octant = bottom_left_front;
                else
                    octant = bottom_left_back;
            }
        }
        else {
            if (pos.y < mid.y) {
                if (pos.z < mid.z)
                    octant = top_right_front;
                else
                    octant = top_right_bottom;
            }
            else {
                if (pos.z < mid.z)
                    octant = bottom_right_front;
                else
                    octant = bottom_right_back;
            }
        }
        return octant;
    }

    bool find(glm::vec3 pos) const {
        if (pos.x < m_top_left_front->x || pos.x > m_bottom_right_back->x ||
            pos.y < m_top_left_front->y || pos.y > m_bottom_right_back->y ||
            pos.z < m_top_left_front->z || pos.z > m_bottom_right_back->z)
            return false;

        const glm::vec3 mid = (*m_top_left_front + *m_bottom_right_back) / 2.f;

        const int octant = get_octant(pos, mid);
        if (m_children[octant]->m_point == nullptr) {
            return m_children[octant]->find(pos);
        }
        else if ((*m_children[octant]->m_point).pos == glm::vec3(-1, -1, -1)) {
            return false;
        }
        else {
            return pos == m_children[octant]->m_point->pos;
        }
    }
};
