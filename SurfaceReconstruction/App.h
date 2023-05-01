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


class application {
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

    static glm::vec3 to_descartes(float fi, float theta);
    static glm::vec3 get_sphere_pos(float u, float v);
    void draw_points(VertexArrayObject& vao, const size_t size);
    void render_imgui();

protected:
    ProgramObject m_axes_program;
    ProgramObject m_particle_program;

    Texture2D m_virtual_camera_textures[3];
    gCamera m_virtual_camera;

    bool m_show_debug_sphere = false;
    std::vector<glm::vec3> m_debug_sphere;
    VertexArrayObject m_gpu_debug_sphere_vao;
    ArrayBuffer m_gpu_debug_sphere_buffer;

    std::vector<file_loader::vertex> m_vertices;
    VertexArrayObject m_gpu_particle_vao;
    ArrayBuffer m_gpu_particle_buffer;
    file_loader::physical_camera_params m_physical_camera_params;

    float m_point_size = 4.f;
    SDL_Window* m_window;
};
