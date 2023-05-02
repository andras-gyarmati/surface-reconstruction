﻿#include "octree.h"
#include <iostream>

octree::octree() {
    m_point = new glm::vec3(-1, -1, -1);
    m_top_left_front = new glm::vec3(0, 0, 0);
    m_bottom_right_back = new glm::vec3(0, 0, 0);
}

octree::octree(glm::vec3 p) {
    m_point = new glm::vec3(p);
    m_top_left_front = new glm::vec3(0, 0, 0);
    m_bottom_right_back = new glm::vec3(0, 0, 0);
}

octree::~octree() {
    delete m_point;
    delete m_top_left_front;
    delete m_bottom_right_back;
    for (const auto child : m_children) {
        delete child;
    }
}

octree::octree(const glm::vec3 tlf, const glm::vec3 brb) {
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

void octree::insert(const glm::vec3 pos) {
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

    int octant = -1;
    if (pos.x <= mid.x) {
        if (pos.y <= mid.y) {
            if (pos.z <= mid.z)
                octant = top_left_front;
            else
                octant = top_left_bottom;
        } else {
            if (pos.z <= mid.z)
                octant = bottom_left_front;
            else
                octant = bottom_left_back;
        }
    } else {
        if (pos.y <= mid.y) {
            if (pos.z <= mid.z)
                octant = top_right_front;
            else
                octant = top_right_bottom;
        } else {
            if (pos.z <= mid.z)
                octant = bottom_right_front;
            else
                octant = bottom_right_back;
        }
    }

    if (m_children[octant]->m_point == nullptr) {
        m_children[octant]->insert(pos);
        return;
    } else if ((*m_children[octant]->m_point) == glm::vec3(-1, -1, -1)) {
        delete m_children[octant];
        m_children[octant] = new octree(pos);
        return;
    } else {
        const glm::vec3 p = *m_children[octant]->m_point;
        delete m_children[octant];
        m_children[octant] = nullptr;
        if (octant == top_left_front) {
            m_children[octant] = new octree(*m_top_left_front, mid);
        } else if (octant == top_right_front) {
            m_children[octant] = new octree(glm::vec3(mid.x + epsilon, m_top_left_front->y, m_top_left_front->z), glm::vec3(m_bottom_right_back->x, mid.y, mid.z));
        } else if (octant == bottom_right_front) {
            m_children[octant] = new octree(glm::vec3(mid.x + epsilon, mid.y + epsilon, m_top_left_front->z), glm::vec3(m_bottom_right_back->x, m_bottom_right_back->y, mid.z));
        } else if (octant == bottom_left_front) {
            m_children[octant] = new octree(glm::vec3(m_top_left_front->x, mid.y + epsilon, m_top_left_front->z), glm::vec3(mid.x, m_bottom_right_back->y, mid.z));
        } else if (octant == top_left_bottom) {
            m_children[octant] = new octree(glm::vec3(m_top_left_front->x, m_top_left_front->y, mid.z + epsilon), glm::vec3(mid.x, mid.y, m_bottom_right_back->z));
        } else if (octant == top_right_bottom) {
            m_children[octant] = new octree(glm::vec3(mid.x + epsilon, m_top_left_front->y, mid.z + epsilon), glm::vec3(m_bottom_right_back->x, mid.y, m_bottom_right_back->z));
        } else if (octant == bottom_right_back) {
            m_children[octant] = new octree(glm::vec3(mid.x + epsilon, mid.y + epsilon, mid.z + epsilon), *m_bottom_right_back);
        } else if (octant == bottom_left_back) {
            m_children[octant] = new octree(glm::vec3(m_top_left_front->x, mid.y + epsilon, mid.z + epsilon), glm::vec3(mid.x, m_bottom_right_back->y, m_bottom_right_back->z));
        }
        m_children[octant]->insert(p);
        m_children[octant]->insert(pos);
    }
}

bool octree::find(glm::vec3 pos) const {
    if (pos.x < m_top_left_front->x || pos.x > m_bottom_right_back->x ||
        pos.y < m_top_left_front->y || pos.y > m_bottom_right_back->y ||
        pos.z < m_top_left_front->z || pos.z > m_bottom_right_back->z)
        return false;

    const glm::vec3 mid = (*m_top_left_front + *m_bottom_right_back) / 2.f;

    int octant = -1;
    if (pos.x <= mid.x) {
        if (pos.y <= mid.y) {
            if (pos.z <= mid.z)
                octant = top_left_front;
            else
                octant = top_left_bottom;
        } else {
            if (pos.z <= mid.z)
                octant = bottom_left_front;
            else
                octant = bottom_left_back;
        }
    } else {
        if (pos.y <= mid.y) {
            if (pos.z <= mid.z)
                octant = top_right_front;
            else
                octant = top_right_bottom;
        } else {
            if (pos.z <= mid.z)
                octant = bottom_right_front;
            else
                octant = bottom_right_back;
        }
    }

    if (m_children[octant]->m_point == nullptr) {
        return m_children[octant]->find(pos);
    } else if ((*m_children[octant]->m_point) == glm::vec3(-1, -1, -1)) {
        return false;
    } else {
        return pos == *m_children[octant]->m_point;
    }
}