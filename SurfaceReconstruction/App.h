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
    double fu, fv, u0, v0;
};

struct device
{
    glm::mat3x3 R;
    std::vector<float> t;
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
    camera_params load_camera_params(const std::string& filename) const;
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
    void mouse_wheel(SDL_MouseWheelEvent&);
    void resize(int, int);

    vertices load_ply_file(const std::string& filename) const;
    static vertices load_xyz_file(const std::string& filename);

protected:
    static glm::vec3 to_descartes(float fi, float theta);

    ProgramObject m_axes_program;
    ProgramObject m_particle_program;
    ProgramObject m_program;

    gCamera m_camera;

    vertices m_vertices;

    VertexArrayObject m_gpu_particle_vao;
    ArrayBuffer m_gpu_particle_buffer;
    camera_params m_camera_params;
};
