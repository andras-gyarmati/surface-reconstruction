// Implementation of Octree in c++
#include <iostream>
#include <vector>
using namespace std;

#define TopLeftFront 0
#define TopRightFront 1
#define BottomRightFront 2
#define BottomLeftFront 3
#define TopLeftBottom 4
#define TopRightBottom 5
#define BottomRightBack 6
#define BottomLeftBack 7

// Octree class
class octree {
    // if point == NULL, node is internal node.
    // if point == (-1, -1, -1), node is empty.
    glm::vec3* point = nullptr;

public:
    // Represent the boundary of the cube
    glm::vec3* m_top_left_front = nullptr;
    glm::vec3* m_bottom_right_back = nullptr;
    vector<octree*> m_children;

    // Constructor
    octree() {
        // To declare empty node
        point = new glm::vec3(-1, -1, -1);
    }

    // Constructor with three arguments
    octree(int x, int y, int z) {
        // To declare point node
        point = new glm::vec3(x, y, z);
    }

    // Constructor with six arguments
    octree(int x1, int y1, int z1, int x2, int y2, int z2) {
        // This use to construct Octree
        // with boundaries defined
        if (x2 < x1
            || y2 < y1
            || z2 < z1) {
            cout << "boundary points are not valid" << endl;
            return;
        }

        point = nullptr;
        m_top_left_front = new glm::vec3(x1, y1, z1);
        m_bottom_right_back = new glm::vec3(x2, y2, z2);

        // Assigning null to the children
        m_children.assign(8, nullptr);
        for (int i = TopLeftFront;
             i <= BottomLeftBack;
             ++i)
            m_children[i] = new octree();
    }

    // Function to insert a point in the octree
    void insert(glm::vec3 pos) {
        // If the point already exists in the octree
        if (find(pos)) {
            cout << "Point already exist in the tree" << endl;
            return;
        }

        // If the point is out of bounds
        if (pos.x < m_top_left_front->x
            || pos.x > m_bottom_right_back->x
            || pos.y < m_top_left_front->y
            || pos.y > m_bottom_right_back->y
            || pos.z < m_top_left_front->z
            || pos.z > m_bottom_right_back->z) {
            cout << "Point is out of bound" << endl;
            return;
        }

        // Binary search to insert the point
        int midx = (m_top_left_front->x
                + m_bottom_right_back->x)
            / 2;
        int midy = (m_top_left_front->y
                + m_bottom_right_back->y)
            / 2;
        int midz = (m_top_left_front->z
                + m_bottom_right_back->z)
            / 2;

        int octant = -1;

        // Checking the octant of
        // the point
        if (pos.x <= midx) {
            if (pos.y <= midy) {
                if (pos.z <= midz)
                    octant = TopLeftFront;
                else
                    octant = TopLeftBottom;
            }
            else {
                if (pos.z <= midz)
                    octant = BottomLeftFront;
                else
                    octant = BottomLeftBack;
            }
        }
        else {
            if (pos.y <= midy) {
                if (pos.z <= midz)
                    octant = TopRightFront;
                else
                    octant = TopRightBottom;
            }
            else {
                if (pos.z <= midz)
                    octant = BottomRightFront;
                else
                    octant = BottomRightBack;
            }
        }

        // If an internal node is encountered
        if (m_children[octant]->point == nullptr) {
            m_children[octant]->insert(pos);
            return;
        }

        // If an empty node is encountered
        else if (m_children[octant]->point->x == -1) {
            delete m_children[octant];
            m_children[octant] = new octree(pos.x, pos.y, pos.z);
            return;
        }
        else {
            glm::vec3 pos_ = *m_children[octant]->point;
            delete m_children[octant];
            m_children[octant] = nullptr;
            if (octant == TopLeftFront) {
                m_children[octant] = new octree(m_top_left_front->x,
                                                m_top_left_front->y,
                                                m_top_left_front->z,
                                                midx,
                                                midy,
                                                midz);
            }

            else if (octant == TopRightFront) {
                m_children[octant] = new octree(midx,
                                                m_top_left_front->y,
                                                m_top_left_front->z,
                                                m_bottom_right_back->x,
                                                midy,
                                                midz);
            }
            else if (octant == BottomRightFront) {
                m_children[octant] = new octree(midx,
                                                midy,
                                                m_top_left_front->z,
                                                m_bottom_right_back->x,
                                                m_bottom_right_back->y,
                                                midz);
            }
            else if (octant == BottomLeftFront) {
                m_children[octant] = new octree(m_top_left_front->x,
                                                midy,
                                                m_top_left_front->z,
                                                midx,
                                                m_bottom_right_back->y,
                                                midz);
            }
            else if (octant == TopLeftBottom) {
                m_children[octant] = new octree(m_top_left_front->x,
                                                m_top_left_front->y,
                                                midz,
                                                midx,
                                                midy,
                                                m_bottom_right_back->z);
            }
            else if (octant == TopRightBottom) {
                m_children[octant] = new octree(midx,
                                                m_top_left_front->y,
                                                midz,
                                                m_bottom_right_back->x,
                                                midy,
                                                m_bottom_right_back->z);
            }
            else if (octant == BottomRightBack) {
                m_children[octant] = new octree(midx,
                                                midy,
                                                midz,
                                                m_bottom_right_back->x,
                                                m_bottom_right_back->y,
                                                m_bottom_right_back->z);
            }
            else if (octant == BottomLeftBack) {
                m_children[octant] = new octree(m_top_left_front->x,
                                                midy,
                                                midz,
                                                midx,
                                                m_bottom_right_back->y,
                                                m_bottom_right_back->z);
            }
            m_children[octant]->insert(pos_);
            m_children[octant]->insert(pos);
            m_children[octant]->point = nullptr;
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
        int midx = (m_top_left_front->x
                + m_bottom_right_back->x)
            / 2;
        int midy = (m_top_left_front->y
                + m_bottom_right_back->y)
            / 2;
        int midz = (m_top_left_front->z
                + m_bottom_right_back->z)
            / 2;

        int octant = -1;

        // Deciding the position
        // where to move
        if (pos.x <= midx) {
            if (pos.y <= midy) {
                if (pos.z <= midz)
                    octant = TopLeftFront;
                else
                    octant = TopLeftBottom;
            }
            else {
                if (pos.z <= midz)
                    octant = BottomLeftFront;
                else
                    octant = BottomLeftBack;
            }
        }
        else {
            if (pos.y <= midy) {
                if (pos.z <= midz)
                    octant = TopRightFront;
                else
                    octant = TopRightBottom;
            }
            else {
                if (pos.z <= midz)
                    octant = BottomRightFront;
                else
                    octant = BottomRightBack;
            }
        }

        // If an internal node is encountered
        if (m_children[octant]->point == nullptr) {
            return m_children[octant]->find(pos);
        }

        // If an empty node is encountered
        else if (m_children[octant]->point->x == -1) {
            return 0;
        }
        else {
            // If node is found with
            // the given value
            if (pos.x == m_children[octant]->point->x
                && pos.y == m_children[octant]->point->y
                && pos.z == m_children[octant]->point->z)
                return 1;
        }
        return 0;
    }
};
