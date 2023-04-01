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

struct vertices
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
};

struct internal_params
{
    float fu, fv, u0, v0;
};

struct device
{
    std::string name;
    glm::mat3 r;
    glm::vec3 t;
};

struct camera_params
{
    internal_params internal_params{};
    std::vector<device> devices;
};

class application
{
public:
    application(void);
    ~application(void) = default;

    bool init();
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

    static vertices load_ply_file(const std::string& filename);
    static vertices load_xyz_file(const std::string& filename);
    static camera_params load_camera_params(const std::string& filename);

    static glm::vec3 to_descartes(float fi, float theta);

    static glm::vec3 get_sphere_pos(float u, float v);

protected:
    ProgramObject m_axes_program;
    ProgramObject m_particle_program;
    ProgramObject m_program;

    Texture2D m_camera_texture;

    gCamera m_camera;

    glm::mat4 m_mat_world = glm::mat4(1.0f);
    glm::mat4 m_mat_view = glm::mat4(1.0f);
    glm::mat4 m_mat_proj = glm::mat4(1.0f);
    float m_fi = static_cast<float>(M_PI) * 1.5f;
    float m_theta = static_cast<float>(M_PI) / 2.0f;
    float m_u = 0.5f;
    float m_v = 0.5f;
    float step_size = 0.01f;
    float m_speed = 0.2f;
    glm::vec3 m_eye = glm::vec3(0, 0, 1);
    glm::vec3 m_fw = to_descartes(m_fi, m_theta);
    glm::vec3 m_at = m_eye + m_fw;
    glm::vec3 m_up = glm::vec3(0, 1, 0);
    glm::vec3 m_left = cross(m_up, m_fw);

    bool m_is_left_pressed = false;


    std::vector<glm::vec3> m_debug_sphere;
    VertexArrayObject m_gpu_debug_sphere_vao;
    ArrayBuffer m_gpu_debug_sphere_buffer;

    vertices m_vertices;
    VertexArrayObject m_gpu_particle_vao;
    ArrayBuffer m_gpu_particle_buffer;
    camera_params m_camera_params;
};
