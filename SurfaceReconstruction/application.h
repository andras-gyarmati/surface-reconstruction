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

class application {
public:
    application(void);
    void init_octree();
    ~application(void) = default;

    bool init(SDL_Window* window);
    void clean();
    auto reset() -> void;

    void update();
    void render();

    void keyboard_down(const SDL_KeyboardEvent&);
    void keyboard_up(const SDL_KeyboardEvent&);
    void mouse_move(const SDL_MouseMotionEvent&);
    void mouse_down(const SDL_MouseButtonEvent&);
    void mouse_up(const SDL_MouseButtonEvent&);
    void mouse_wheel(const SDL_MouseWheelEvent&);
    void resize(int, int);
    void load_inputs_from_folder(const std::string& folder_name);
    void init_debug_sphere();
    void init_box(const glm::vec3& top_left_front, const glm::vec3& bottom_right_back);
    void draw_points(VertexArrayObject& vao, size_t size);
    void render_imgui();
    void init_octree_visualization(const octree* root);
    void render_octree_boxes();
    void init_mesh_visualization();
    void render_mesh();
    void randomize_colors();
    glm::vec3 hsl_to_rgb(float h, float s, float l) const;
    glm::vec3 get_random_color() const;

    glm::vec3 get_sphere_pos(const float u, const float v) {
        const float th = u * 2.0f * static_cast<float>(M_PI);
        const float fi = v * 2.0f * static_cast<float>(M_PI);
        constexpr float r = 2.0f;

        return {
            r * sin(th) * cos(fi),
            r * sin(th) * sin(fi),
            r * cos(th)
        };
    }

protected:
    ProgramObject m_axes_program;

    file_loader::digital_camera_params m_digital_camera_params;
    Texture2D m_digital_camera_textures[3];
    gCamera m_virtual_camera;

    ProgramObject m_particle_program;
    std::vector<file_loader::vertex> m_vertices;
    VertexArrayObject m_gpu_particle_vao;
    ArrayBuffer m_gpu_particle_buffer;
    int m_render_points_up_to_index;

    bool m_show_debug_sphere;
    std::vector<glm::vec3> m_debug_sphere;
    VertexArrayObject m_gpu_debug_sphere_vao;
    ArrayBuffer m_gpu_debug_sphere_buffer;

    bool m_show_octree;
    octree m_octree;
    int m_points_to_add_index{};
    int m_points_added_index{};
    std::vector<glm::vec3> m_box_pos;
    std::vector<int> m_box_indices;
    ProgramObject m_box_wireframe_program;
    VertexArrayObject m_box_vao;
    IndexBuffer m_box_indices_gpu_buffer;
    ArrayBuffer m_box_pos_gpu_buffer;

    std::vector<file_loader::vertex> m_mesh_vertices;
    std::vector<int> m_mesh_indices;
    ProgramObject m_mesh_program;
    VertexArrayObject m_mesh_vao;
    IndexBuffer m_mesh_indices_gpu_buffer;
    ArrayBuffer m_mesh_pos_gpu_buffer;

    bool m_auto_increment_rendered_point_index;
    float m_point_size;
    SDL_Window* m_window{};
    char m_input_folder[256]{};
    float m_ignore_center_radius;
    int m_mesh_rendering_mode;
};
