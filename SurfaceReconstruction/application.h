#pragma once
#include <SDL.h>
#include <glm/glm.hpp>
#include "Includes/ProgramObject.h"
#include "Includes/BufferObject.h"
#include "Includes/VertexArrayObject.h"
#include "Includes/TextureObject.h"
#include "Includes/gCamera.h"
#include <vector>
#include "delaunay_3d.h"
#include "file_loader.h"
#include "octree.h"
#include <cstdlib>

enum mesh_rendering_mode {
    none = 0,
    wireframe = 1,
    solid = 2
};

class application {
public:
    // constructor destructor
    application(void);
    ~application(void) = default;

    // application lifecycle methods
    bool init(SDL_Window* window);
    void clean();
    void reset();
    void update();
    void render();

    // user io event handling methods
    void keyboard_down(const SDL_KeyboardEvent&);
    void keyboard_up(const SDL_KeyboardEvent&);
    void mouse_move(const SDL_MouseMotionEvent&);
    void mouse_down(const SDL_MouseButtonEvent&);
    void mouse_up(const SDL_MouseButtonEvent&);
    void mouse_wheel(const SDL_MouseWheelEvent&);
    void resize(int, int);

    // file input
    void load_inputs_from_folder(const std::string& folder_name);

    // init methods
    void init_point_visualization();
    void init_debug_sphere();
    void init_octree(const std::vector<file_loader::vertex>& vertices);
    static void init_box(const glm::vec3& tlf, const glm::vec3& brb, std::vector<file_loader::vertex>& vertices, std::vector<int>& indices, glm::vec3 color);
    void init_octree_visualization(const octree* root);
    void init_mesh_visualization();
    void init_sensor_rig_boundary_visualization();
    void init_delaunay_shaded_points_segment();
    void init_delaunay_cube();
    void init_delaunay();
    void init_delaunay_visualization();
    void init_tetrahedron(const delaunay_3d::tetrahedron* tetrahedron);

    // render methods
    void render_imgui();
    void render_points(VertexArrayObject& vao, size_t size);
    void render_octree_boxes();
    void render_mesh();
    void render_sensor_rig_boundary();
    void render_tetrahedra();

    // helper functions
    static std::vector<file_loader::vertex> get_cube_vertices(float side_len);
    std::vector<file_loader::vertex> filter_shaded_points(const std::vector<file_loader::vertex>& points);
    bool is_mesh_vertex_cut_distance_ok(int i0, int i1, int i2) const;
    bool is_outside_of_sensor_rig_boundary(int i0, int i1, int i2) const;
    void set_particle_program_uniforms(bool show_non_shaded);
    void randomize_vertex_colors(std::vector<file_loader::vertex>& vertices) const;
    glm::vec3 hsl_to_rgb(float h, float s, float l) const;
    glm::vec3 get_random_color() const;
    glm::vec3 get_sphere_pos(float u, float v) const;
    static void toggle_fullscreen(SDL_Window* win);

    float* EstimatePlaneImplicit(const std::vector<glm::vec3>& pts) {
        int num = pts.size();

        glm::mat4 Cfs(0.0f);

        for (const auto& pt : pts) {
            glm::vec4 row(pt.x, pt.y, pt.z, 1.0f);
            Cfs += glm::outerProduct(row, row);
        }

        glm::vec4 guess(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 eigvec(1.0f, 1.0f, 1.0f, 1.0f);

        for (int iter = 0; iter < 100; ++iter) {
            eigvec = Cfs * guess;
            guess = glm::normalize(eigvec);
        }

        float A = guess.x;
        float B = guess.y;
        float C = guess.z;
        float D = guess.w;

        float norm = std::sqrt(A * A + B * B + C * C);

        float* ret = new float[4];
        ret[0] = A / norm;
        ret[1] = B / norm;
        ret[2] = C / norm;
        ret[3] = D / norm;

        return ret;
    }


    struct RANSACDiffs {
        int inliersNum;
        std::vector<bool> isInliers;
        std::vector<float> distances;
    };


    RANSACDiffs PlanePointRANSACDifferences(const std::vector<glm::vec3>& pts, float* plane, float threshold) {
        int num = pts.size();

        float A = plane[0];
        float B = plane[1];
        float C = plane[2];
        float D = plane[3];

        RANSACDiffs ret;

        std::vector<bool> isInliers;
        std::vector<float> distances;

        int inlierCounter = 0;
        for (int idx = 0; idx < num; idx++) {
            glm::vec3 pt = pts.at(idx);
            float diff = std::fabs(A * pt.x + B * pt.y + C * pt.z + D);
            distances.push_back(diff);
            if (diff < threshold) {
                isInliers.push_back(true);
                ++inlierCounter;
            } else {
                isInliers.push_back(false);
            }
        }

        ret.distances = distances;
        ret.isInliers = isInliers;
        ret.inliersNum = inlierCounter;

        return ret;
    }

    float* EstimatePlaneRANSAC(const std::vector<glm::vec3>& pts, float threshold, int iterNum) {
        int num = pts.size();

        int bestSampleInlierNum = 0;
        float bestPlane[4];

        for (int iter = 0; iter < iterNum; iter++) {
            float rand1 = static_cast<float>(rand()) / RAND_MAX;
            float rand2 = static_cast<float>(rand()) / RAND_MAX;
            float rand3 = static_cast<float>(rand()) / RAND_MAX;

            int index1 = static_cast<int>(rand1 * num);
            int index2 = static_cast<int>(rand2 * num);
            while (index2 == index1) {
                rand2 = static_cast<float>(rand()) / RAND_MAX;
                index2 = static_cast<int>(rand2 * num);
            }
            int index3 = static_cast<int>(rand3 * num);
            while (index3 == index1 || index3 == index2) {
                rand3 = static_cast<float>(rand()) / RAND_MAX;
                index3 = static_cast<int>(rand3 * num);
            }

            glm::vec3 pt1 = pts.at(index1);
            glm::vec3 pt2 = pts.at(index2);
            glm::vec3 pt3 = pts.at(index3);

            std::vector<glm::vec3> minimalSample;
            minimalSample.push_back(pt1);
            minimalSample.push_back(pt2);
            minimalSample.push_back(pt3);

            float* samplePlane = EstimatePlaneImplicit(minimalSample);

            RANSACDiffs sampleResult = PlanePointRANSACDifferences(pts, samplePlane, threshold);

            if (sampleResult.inliersNum > bestSampleInlierNum) {
                bestSampleInlierNum = sampleResult.inliersNum;
                for (int i = 0; i < 4; ++i) {
                    bestPlane[i] = samplePlane[i];
                }
            }

            delete[] samplePlane;
        }

        RANSACDiffs bestResult = PlanePointRANSACDifferences(pts, bestPlane, threshold);

        std::vector<glm::vec3> inlierPts;
        for (int idx = 0; idx < num; idx++) {
            if (bestResult.isInliers.at(idx)) {
                inlierPts.push_back(pts.at(idx));
            }
        }

        float* finalPlane = EstimatePlaneImplicit(inlierPts);
        return finalPlane;
    }


    void RunRANSAC(const std::vector<glm::vec3>& points) {
        // Constants, replace them as needed
        const float FILTER_LOWEST_DISTANCE = 0.1f;
        const float THERSHOLD = 0.5f;
        const int RANSAC_ITER = 100;

        std::vector<glm::vec3> filteredPoints = points;

        // for (const auto& point : points) {
        //     float distFromOrigo = glm::length(point);
        //
        //     if (distFromOrigo > FILTER_LOWEST_DISTANCE) {
        //         filteredPoints.push_back(point);
        //     }
        // }

        int num = filteredPoints.size();

        float* plane = EstimatePlaneImplicit(filteredPoints);

        std::cout << "Plane fitting results from the whole data:" << std::endl;
        std::cout << "A:" << plane[0] << " B:" << plane[1]
            << " C:" << plane[2] << " D:" << plane[3] << std::endl;

        delete[] plane;

        float* planeParams = EstimatePlaneRANSAC(filteredPoints, THERSHOLD, RANSAC_ITER);

        std::cout << "Plane params RANSAC:" << std::endl;
        std::cout << "A:" << planeParams[0] << " B:" << planeParams[1]
            << " C:" << planeParams[2] << " D:" << planeParams[3] << std::endl;

        RANSACDiffs differences = PlanePointRANSACDifferences(filteredPoints, planeParams, THERSHOLD);

        delete[] planeParams;

        // Assuming you have a WritePLY function that accepts glm::vec3 and glm::ivec3
        std::vector<glm::ivec3> colorsRANSAC;

        for (int idx = 0; idx < num; ++idx) {
            glm::ivec3 newColor;

            if (differences.isInliers.at(idx)) {
                newColor = glm::ivec3(0, 255, 0);
            } else {
                newColor = glm::ivec3(255, 0, 0);
            }

            colorsRANSAC.push_back(newColor);
        }

        // Replace with your own function to write the results
        // WritePLY("output.ply", filteredPoints, colorsRANSAC);
    }

protected:
    // shader programs
    ProgramObject m_axes_program;
    ProgramObject m_particle_program;
    ProgramObject m_wireframe_program;

    // VAOs
    VertexArrayObject m_particle_vao;
    VertexArrayObject m_debug_sphere_vao;
    VertexArrayObject m_wireframe_vao;
    VertexArrayObject m_sensor_rig_boundary_vao;
    VertexArrayObject m_tetrahedra_vao;
    VertexArrayObject m_mesh_vao;

    // array buffers
    ArrayBuffer m_particle_buffer;
    ArrayBuffer m_debug_sphere_buffer;
    ArrayBuffer m_wireframe_vertices_buffer;
    ArrayBuffer m_sensor_rig_boundary_vertices_buffer;
    ArrayBuffer m_tetrahedra_vertices_buffer;
    ArrayBuffer m_mesh_pos_buffer;

    // index buffers
    IndexBuffer m_wireframe_indices_buffer;
    IndexBuffer m_sensor_rig_boundary_indices_buffer;
    IndexBuffer m_tetrahedra_indices_buffer;
    IndexBuffer m_mesh_indices_buffer;

    // index vectors
    std::vector<int> m_wireframe_indices;
    std::vector<int> m_sensor_rig_boundary_indices;
    std::vector<int> m_tetrahedra_indices;
    std::vector<int> m_mesh_indices;

    // vertex vectors
    std::vector<file_loader::vertex> m_vertices;
    std::vector<file_loader::vertex> m_delaunay_vertices;
    std::vector<file_loader::vertex> m_wireframe_vertices;
    std::vector<file_loader::vertex> m_sensor_rig_boundary_vertices;
    std::vector<file_loader::vertex> m_tetrahedra_vertices;

    // flags
    bool m_show_axes;
    bool m_show_points;
    bool m_show_debug_sphere;
    bool m_show_octree;
    bool m_show_sensor_rig_boundary;
    bool m_show_tetrahedra;
    bool m_show_back_faces;
    bool m_show_non_shaded_points;
    bool m_show_non_shaded_mesh;
    bool m_auto_increment_rendered_point_index;

    // numeric values
    int m_render_points_up_to_index;
    int m_debug_sphere_n = 959;
    int m_debug_sphere_m = 959;
    float m_point_size;
    float m_mesh_vertex_cut_distance;
    float m_line_width;

    // other objects
    SDL_Window* m_window{};
    gCamera m_virtual_camera;
    glm::vec3 m_start_eye;
    glm::vec3 m_start_at;
    glm::vec3 m_start_up;
    glm::vec3 m_octree_color;
    octree m_octree;
    delaunay_3d m_delaunay;
    octree::boundary m_sensor_rig_boundary;
    mesh_rendering_mode m_mesh_rendering_mode;
    file_loader::digital_camera_params m_digital_camera_params;
    char m_input_folder[256]{};
    Texture2D m_digital_camera_textures[3];
    std::vector<glm::vec3> m_debug_sphere;
};
