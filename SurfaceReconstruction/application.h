#pragma once
#include <SDL.h>
#include <glm/glm.hpp>
#include "Includes/ProgramObject.h"
#include "Includes/BufferObject.h"
#include "Includes/VertexArrayObject.h"
#include "Includes/TextureObject.h"
#include "Includes/gCamera.h"
#include <vector>
#include "file_loader.h"
#include "octree.h"

enum mesh_rendering_mode {
    none = 0,
    wireframe = 1,
    solid = 2
};

struct cut {
    glm::vec2 uv;
    float dist = std::numeric_limits<float>::min();
    float uv_dist;
    float ratio;
    float x;
    glm::vec3 normal;
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

    // render methods
    void render_imgui();
    void render_points(VertexArrayObject& vao, size_t size);
    void render_octree_boxes();
    void render_mesh();
    void render_sensor_rig_boundary();

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

    std::vector<file_loader::vertex> set_uvs(std::vector<file_loader::vertex>& points) {
        std::vector<file_loader::vertex> shaded_points;
        glm::mat3 cam_k = m_digital_camera_params.get_cam_k();
        glm::mat3 cam_r[3];
        glm::vec3 cam_t[3];
        cam_r[0] = m_digital_camera_params.devices[0].r;
        cam_r[1] = m_digital_camera_params.devices[1].r;
        cam_r[2] = m_digital_camera_params.devices[2].r;
        cam_t[0] = m_digital_camera_params.devices[0].t;
        cam_t[1] = m_digital_camera_params.devices[1].t;
        cam_t[2] = m_digital_camera_params.devices[2].t;

        for (int j = 0; j < points.size(); ++j) {
            auto& point = points[j];
            bool is_shaded = false;
            for (int i = 0; i < 3; ++i) {
                glm::vec3 p_tmp = cam_r[i] * (point.position - cam_t[i]);
                const float dist = p_tmp.z;
                p_tmp /= p_tmp.z;
                glm::vec2 p_c;
                p_c.x = cam_k[0][0] * p_tmp.x + cam_k[0][2];
                p_c.y = cam_k[1][1] * -p_tmp.y + cam_k[1][2];
                // todo: get triangles uv coords and check if triangle is too stretched
                // based on distance we can map the triangle sizes and orientations
                // to a lower dimension and check for outliers
                if (dist > 0 && p_c.x >= 0 && p_c.x <= 960 && p_c.y >= 0 && p_c.y <= 600) {
                    is_shaded = true;
                    // put uv into point
                    m_cuts[j].uv = p_c; // todo: compensate for the 3 different cameras
                }
            }
            if (is_shaded && !m_sensor_rig_boundary.contains(point.position)) {
                shaded_points.push_back(point);
            }
        }
        return shaded_points;
    }

    // if triangle is too stretched, it is probably a bad triangle
    bool is_triangle_should_be_excluded(const int a, const int b, const int c) {
        float dist_a_b = glm::distance(m_vertices[a].position, m_vertices[b].position);
        float dist_b_c = glm::distance(m_vertices[b].position, m_vertices[c].position);
        float dist_c_a = glm::distance(m_vertices[c].position, m_vertices[a].position);
        float uv_dist_a_b = glm::distance(m_cuts[a].uv, m_cuts[b].uv);
        float uv_dist_b_c = glm::distance(m_cuts[b].uv, m_cuts[c].uv);
        float uv_dist_c_a = glm::distance(m_cuts[c].uv, m_cuts[a].uv);
        glm::vec3 normal_a = glm::normalize(glm::cross(m_vertices[b].position - m_vertices[a].position, m_vertices[c].position - m_vertices[a].position));
        glm::vec3 normal_b = glm::normalize(glm::cross(m_vertices[c].position - m_vertices[b].position, m_vertices[a].position - m_vertices[b].position));
        glm::vec3 normal_c = glm::normalize(glm::cross(m_vertices[a].position - m_vertices[c].position, m_vertices[b].position - m_vertices[c].position));
        m_cuts[a].normal = normal_a;
        m_cuts[b].normal = normal_b;
        m_cuts[c].normal = normal_c;
        m_vertices[a].normal = normal_a;
        m_vertices[b].normal = normal_b;
        m_vertices[c].normal = normal_c;
        if (dist_a_b < m_min_dist) m_min_dist = dist_a_b;
        if (dist_a_b > m_max_dist) m_max_dist = dist_a_b;
        if (dist_b_c < m_min_dist) m_min_dist = dist_b_c;
        if (dist_b_c > m_max_dist) m_max_dist = dist_b_c;
        if (dist_c_a < m_min_dist) m_min_dist = dist_c_a;
        if (dist_c_a > m_max_dist) m_max_dist = dist_c_a;
        float dist_a_b_uv_dist_ratio = uv_dist_a_b / dist_a_b;
        if (dist_a_b_uv_dist_ratio < m_min_v_dist_uv_dist_ratio) m_min_v_dist_uv_dist_ratio = dist_a_b_uv_dist_ratio;
        if (dist_a_b_uv_dist_ratio > m_max_v_dist_uv_dist_ratio) m_max_v_dist_uv_dist_ratio = dist_a_b_uv_dist_ratio;
        float dist_b_c_uv_dist_ratio = uv_dist_b_c / dist_b_c;
        if (dist_b_c_uv_dist_ratio < m_min_v_dist_uv_dist_ratio) m_min_v_dist_uv_dist_ratio = dist_b_c_uv_dist_ratio;
        if (dist_b_c_uv_dist_ratio > m_max_v_dist_uv_dist_ratio) m_max_v_dist_uv_dist_ratio = dist_b_c_uv_dist_ratio;
        float dist_c_a_uv_dist_ratio = uv_dist_c_a / dist_c_a;
        if (dist_c_a_uv_dist_ratio < m_min_v_dist_uv_dist_ratio) m_min_v_dist_uv_dist_ratio = dist_c_a_uv_dist_ratio;
        if (dist_c_a_uv_dist_ratio > m_max_v_dist_uv_dist_ratio) m_max_v_dist_uv_dist_ratio = dist_c_a_uv_dist_ratio;
        if (m_cuts[a].dist < dist_a_b) { m_cuts[a].dist = dist_a_b; m_cuts[a].uv_dist = uv_dist_a_b; m_cuts[a].ratio = dist_a_b_uv_dist_ratio; }
        if (m_cuts[b].dist < dist_a_b) { m_cuts[b].dist = dist_a_b; m_cuts[b].uv_dist = uv_dist_a_b; m_cuts[b].ratio = dist_a_b_uv_dist_ratio; }
        if (m_cuts[b].dist < dist_b_c) { m_cuts[b].dist = dist_b_c; m_cuts[b].uv_dist = uv_dist_b_c; m_cuts[b].ratio = dist_b_c_uv_dist_ratio; }
        if (m_cuts[c].dist < dist_b_c) { m_cuts[c].dist = dist_b_c; m_cuts[c].uv_dist = uv_dist_b_c; m_cuts[c].ratio = dist_b_c_uv_dist_ratio; }
        if (m_cuts[c].dist < dist_c_a) { m_cuts[c].dist = dist_c_a; m_cuts[c].uv_dist = uv_dist_c_a; m_cuts[c].ratio = dist_c_a_uv_dist_ratio; }
        if (m_cuts[a].dist < dist_c_a) { m_cuts[a].dist = dist_c_a; m_cuts[a].uv_dist = uv_dist_c_a; m_cuts[a].ratio = dist_c_a_uv_dist_ratio; }
        // write values to console
        // std::cout << "dist_a_b: " << dist_a_b << std::endl;
        // std::cout << "dist_b_c: " << dist_b_c << std::endl;
        // std::cout << "dist_c_a: " << dist_c_a << std::endl;
        // std::cout << "uv_dist_a_b: " << uv_dist_a_b << std::endl;
        // std::cout << "uv_dist_b_c: " << uv_dist_b_c << std::endl;
        // std::cout << "uv_dist_c_a: " << uv_dist_c_a << std::endl << std::endl;
        return false; //dist_a_b > 0.0f && dist_b_c > 0.0f && dist_c_a > 0.0f && uv_dist_a_b == 0.0f && uv_dist_b_c == 0.0f && uv_dist_c_a == 0.0f;
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
    VertexArrayObject m_mesh_vao;

    // array buffers
    ArrayBuffer m_particle_buffer;
    ArrayBuffer m_debug_sphere_buffer;
    ArrayBuffer m_wireframe_vertices_buffer;
    ArrayBuffer m_sensor_rig_boundary_vertices_buffer;
    ArrayBuffer m_mesh_pos_buffer;

    // index buffers
    IndexBuffer m_wireframe_indices_buffer;
    IndexBuffer m_sensor_rig_boundary_indices_buffer;
    IndexBuffer m_mesh_indices_buffer;

    // index vectors
    std::vector<int> m_wireframe_indices;
    std::vector<int> m_sensor_rig_boundary_indices;
    std::vector<int> m_mesh_indices;

    // vertex vectors
    std::vector<file_loader::vertex> m_vertices;
    std::vector<cut> m_cuts;
    float m_min_dist = std::numeric_limits<float>::max();
    float m_max_dist = std::numeric_limits<float>::min();
    float m_min_v_dist_uv_dist_ratio = std::numeric_limits<float>::max();
    float m_max_v_dist_uv_dist_ratio = std::numeric_limits<float>::min();
    float m_max_dist_from_center = std::numeric_limits<float>::min();
    std::vector<file_loader::vertex> m_wireframe_vertices;
    std::vector<file_loader::vertex> m_sensor_rig_boundary_vertices;

    // flags
    bool m_show_axes;
    bool m_show_points;
    bool m_show_debug_sphere;
    bool m_show_octree;
    bool m_show_sensor_rig_boundary;
    bool m_show_normal;
    bool m_show_color;
    bool m_show_back_faces;
    bool m_show_non_shaded_points;
    bool m_show_non_shaded_mesh;
    bool m_auto_increment_rendered_point_index;

    // numeric values
    int m_render_points_up_to_index;
    int m_debug_sphere_n = 959;
    int m_debug_sphere_m = 959;
    float m_point_size;
    float m_cut_scalar;
    float m_cut_scalar2;
    float m_line_width;

    // other objects
    SDL_Window* m_window{};
    gCamera m_virtual_camera;
    glm::vec3 m_start_eye;
    glm::vec3 m_start_at;
    glm::vec3 m_start_up;
    glm::vec3 m_octree_color;
    octree m_octree;
    octree::boundary m_sensor_rig_boundary;
    mesh_rendering_mode m_mesh_rendering_mode;
    file_loader::digital_camera_params m_digital_camera_params;
    char m_input_folder[256]{};
    Texture2D m_digital_camera_textures[3];
    std::vector<glm::vec3> m_debug_sphere;
};
