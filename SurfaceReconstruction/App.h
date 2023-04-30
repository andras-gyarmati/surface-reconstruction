#pragma once

// C++ includes
#include <memory>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL.h>
#include <SDL_opengl.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

#include "Includes/ProgramObject.h"
#include "Includes/BufferObject.h"
#include "Includes/VertexArrayObject.h"
#include "Includes/TextureObject.h"

#include "Includes/Mesh_OGL3.h"
#include "Includes/gCamera.h"

#include <vector>

struct vertex {
    glm::vec3 position;
    glm::vec3 color;
};

struct internal_params {
    float fu, fv, u0, v0;
};

struct device {
    std::string name;
    glm::mat3 r;
    glm::vec3 t;
};

struct camera_params {
    internal_params internal_params{};
    std::vector<device> devices;
};

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

    static std::vector<vertex> load_ply_file(const std::string& filename);
    static std::vector<vertex> read_vertices_from_file(std::ifstream* file, int num_vertices);
    static std::vector<vertex> load_xyz_file(const std::string& filename);
    static camera_params load_camera_params(const std::string& filename);
    static glm::vec3 to_descartes(float fi, float theta);
    static glm::vec3 get_sphere_pos(float u, float v);
    void draw_points(VertexArrayObject& vao, const size_t size);

protected:
    ProgramObject m_axes_program;
    ProgramObject m_particle_program;

    Texture2D m_camera_texture;
    gCamera m_virtual_camera;

    bool m_show_debug_sphere = false;
    std::vector<glm::vec3> m_debug_sphere;
    VertexArrayObject m_gpu_debug_sphere_vao;
    ArrayBuffer m_gpu_debug_sphere_buffer;

    std::vector<vertex> m_vertices;
    VertexArrayObject m_gpu_particle_vao;
    ArrayBuffer m_gpu_particle_buffer;
    camera_params m_camera_params;

    float m_point_size = 4.f;
    SDL_Window* m_window;
};
