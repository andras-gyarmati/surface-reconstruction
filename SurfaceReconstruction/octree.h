// Implementation of Octree in c++
#pragma once
#include <iostream>
#include <vector>

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

// Octree class
class octree {
    // if point == NULL, node is internal node.
    // if point == (-1, -1, -1), node is empty.
    glm::vec3* m_point = nullptr;

public:
    // Represent the boundary of the cube
    glm::vec3* m_top_left_front = nullptr;
    glm::vec3* m_bottom_right_back = nullptr;
    std::vector<octree*> m_children;

    // Constructor
    octree() {
        // To declare empty node
        m_point = new glm::vec3(-1, -1, -1);
    }

    // Constructor with three arguments
    octree(float x, float y, float z) {
        // To declare point node
        m_point = new glm::vec3(x, y, z);
    }

    // Constructor with six arguments
    octree(float x1, float y1, float z1, float x2, float y2, float z2) {
        // This use to construct Octree
        // with boundaries defined
        if (x2 < x1
            || y2 < y1
            || z2 < z1) {
            std::cout << "boundary points are not valid" << std::endl;
            return;
        }

        m_point = nullptr;
        m_top_left_front = new glm::vec3(x1, y1, z1);
        m_bottom_right_back = new glm::vec3(x2, y2, z2);

        // Assigning null to the children
        m_children.assign(8, nullptr);
        for (int i = top_left_front;
             i <= bottom_left_back;
             ++i)
            m_children[i] = new octree();
    }

    // Function to insert a point in the octree
    void insert(glm::vec3 pos) {
        // If the point already exists in the octree
        if (find(pos)) {
            std::cout << "Point already exist in the tree" << std::endl;
            return;
        }

        // If the point is out of bounds
        if (pos.x < m_top_left_front->x
            || pos.x > m_bottom_right_back->x
            || pos.y < m_top_left_front->y
            || pos.y > m_bottom_right_back->y
            || pos.z < m_top_left_front->z
            || pos.z > m_bottom_right_back->z) {
            std::cout << "Point is out of bound" << std::endl;
            return;
        }

        // Binary search to insert the point
        float midx = (m_top_left_front->x
                + m_bottom_right_back->x)
            / 2;
        float midy = (m_top_left_front->y
                + m_bottom_right_back->y)
            / 2;
        float midz = (m_top_left_front->z
                + m_bottom_right_back->z)
            / 2;

        int octant = -1;

        // Checking the octant of
        // the point
        if (pos.x <= midx) {
            if (pos.y <= midy) {
                if (pos.z <= midz)
                    octant = top_left_front;
                else
                    octant = top_left_bottom;
            }
            else {
                if (pos.z <= midz)
                    octant = bottom_left_front;
                else
                    octant = bottom_left_back;
            }
        }
        else {
            if (pos.y <= midy) {
                if (pos.z <= midz)
                    octant = top_right_front;
                else
                    octant = top_right_bottom;
            }
            else {
                if (pos.z <= midz)
                    octant = bottom_right_front;
                else
                    octant = bottom_right_back;
            }
        }

        // If an internal node is encountered
        if (m_children[octant]->m_point == nullptr) {
            m_children[octant]->insert(pos);
            return;
        }

        // If an empty node is encountered
        else if (m_children[octant]->m_point->x == -1) {
            delete m_children[octant];
            m_children[octant] = new octree(pos.x, pos.y, pos.z);
            return;
        }
        else {
            glm::vec3 pos_ = *m_children[octant]->m_point;
            delete m_children[octant];
            m_children[octant] = nullptr;
            if (octant == top_left_front) {
                m_children[octant] = new octree(m_top_left_front->x,
                                                m_top_left_front->y,
                                                m_top_left_front->z,
                                                midx,
                                                midy,
                                                midz);
            }

            else if (octant == top_right_front) {
                m_children[octant] = new octree(midx,
                                                m_top_left_front->y,
                                                m_top_left_front->z,
                                                m_bottom_right_back->x,
                                                midy,
                                                midz);
            }
            else if (octant == bottom_right_front) {
                m_children[octant] = new octree(midx,
                                                midy,
                                                m_top_left_front->z,
                                                m_bottom_right_back->x,
                                                m_bottom_right_back->y,
                                                midz);
            }
            else if (octant == bottom_left_front) {
                m_children[octant] = new octree(m_top_left_front->x,
                                                midy,
                                                m_top_left_front->z,
                                                midx,
                                                m_bottom_right_back->y,
                                                midz);
            }
            else if (octant == top_left_bottom) {
                m_children[octant] = new octree(m_top_left_front->x,
                                                m_top_left_front->y,
                                                midz,
                                                midx,
                                                midy,
                                                m_bottom_right_back->z);
            }
            else if (octant == top_right_bottom) {
                m_children[octant] = new octree(midx,
                                                m_top_left_front->y,
                                                midz,
                                                m_bottom_right_back->x,
                                                midy,
                                                m_bottom_right_back->z);
            }
            else if (octant == bottom_right_back) {
                m_children[octant] = new octree(midx,
                                                midy,
                                                midz,
                                                m_bottom_right_back->x,
                                                m_bottom_right_back->y,
                                                m_bottom_right_back->z);
            }
            else if (octant == bottom_left_back) {
                m_children[octant] = new octree(m_top_left_front->x,
                                                midy,
                                                midz,
                                                midx,
                                                m_bottom_right_back->y,
                                                m_bottom_right_back->z);
            }
            m_children[octant]->insert(pos_);
            m_children[octant]->insert(pos);
            m_children[octant]->m_point = nullptr;
        }
    }

    // Function that returns true if the point
    // (x, y, z) exists in the octree
    bool find(glm::vec3 pos) {
        // If point is out of bound
        if (pos.x < m_top_left_front->x
            || pos.x > m_bottom_right_back->x
            || pos.y < m_top_left_front->y
            || pos.y > m_bottom_right_back->y
            || pos.z < m_top_left_front->z
            || pos.z > m_bottom_right_back->z)
            return 0;

        // Otherwise perform binary search
        // for each ordinate
        float midx = (m_top_left_front->x
                + m_bottom_right_back->x)
            / 2;
        float midy = (m_top_left_front->y
                + m_bottom_right_back->y)
            / 2;
        float midz = (m_top_left_front->z
                + m_bottom_right_back->z)
            / 2;

        int octant = -1;

        // Deciding the position
        // where to move
        if (pos.x <= midx) {
            if (pos.y <= midy) {
                if (pos.z <= midz)
                    octant = top_left_front;
                else
                    octant = top_left_bottom;
            }
            else {
                if (pos.z <= midz)
                    octant = bottom_left_front;
                else
                    octant = bottom_left_back;
            }
        }
        else {
            if (pos.y <= midy) {
                if (pos.z <= midz)
                    octant = top_right_front;
                else
                    octant = top_right_bottom;
            }
            else {
                if (pos.z <= midz)
                    octant = bottom_right_front;
                else
                    octant = bottom_right_back;
            }
        }

        // If an internal node is encountered
        if (m_children[octant]->m_point == nullptr) {
            return m_children[octant]->find(pos);
        }

        // If an empty node is encountered
        else if (m_children[octant]->m_point->x == -1) {
            return 0;
        }
        else {
            // If node is found with
            // the given value
            if (pos.x == m_children[octant]->m_point->x
                && pos.y == m_children[octant]->m_point->y
                && pos.z == m_children[octant]->m_point->z)
                return 1;
        }
        return 0;
    }
};
