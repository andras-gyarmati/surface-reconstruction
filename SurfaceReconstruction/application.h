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

class application
{
public:
    application(void);
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
    void draw_points(VertexArrayObject& vao, size_t size);
    void render_imgui();

    glm::vec3 get_sphere_pos(const float u, const float v)
    {
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
    ProgramObject m_particle_program;
    ProgramObject m_box_wireframe_program;

    Texture2D m_digital_camera_textures[3];
    gCamera m_virtual_camera;

    bool m_show_debug_sphere = false;
    std::vector<glm::vec3> m_debug_sphere;
    VertexArrayObject m_gpu_debug_sphere_vao;
    ArrayBuffer m_gpu_debug_sphere_buffer;

    std::vector<file_loader::vertex> m_vertices;
    VertexArrayObject m_gpu_particle_vao;
    ArrayBuffer m_gpu_particle_buffer;
    file_loader::digital_camera_params m_digital_camera_params;

    VertexArrayObject m_octree_vao;
    ArrayBuffer m_octree_buffer;

    float m_point_size = 4.f;
    SDL_Window* m_window{};
    char m_input_folder[256]{};
};
