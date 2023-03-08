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

struct Vertices {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
};

class CMyApp
{
public:
    CMyApp(void);
    ~CMyApp(void);

    bool Init();
    void Clean();
    void Reset();

    void Update();
    void Render();

    void KeyboardDown(SDL_KeyboardEvent&);
    void KeyboardUp(SDL_KeyboardEvent&);
    void MouseMove(SDL_MouseMotionEvent&);
    void MouseDown(SDL_MouseButtonEvent&);
    void MouseUp(SDL_MouseButtonEvent&);
    void MouseWheel(SDL_MouseWheelEvent&);
    void Resize(int, int);

    Vertices loadPLYFile(const std::string& filename);

protected:
    // belső eljárások, segédfüggvények
    glm::vec3 GetSpherePos(float u, float v);
    glm::vec3 ToDescartes(float fi, float theta);

    // shaderekhez szükséges változók
    ProgramObject		m_passthroughProgram;
    ProgramObject		m_axesProgram;
    ProgramObject		m_particleProgram;
    ProgramObject		m_program;

    VertexArrayObject	m_vao;				// VAO objektum
    IndexBuffer			m_gpuBufferIndices;	// indexek
    ArrayBuffer			m_gpuBufferPos;		// pozíciók tömbje

    // gCamera				m_camera;

    // transzformációs mátrixok
    glm::mat4 m_matWorld = glm::mat4(1.0f);
    glm::mat4 m_matView = glm::mat4(1.0f);
    glm::mat4 m_matProj = glm::mat4(1.0f);

    int	m_particleCount = 150;
    float a = 0.63f;
    float b = 0.41f;
    float c = 0.27f;
    float max_speed = 0.7f;
    float n_dist = 0.3f;

    std::vector<glm::vec3> m_particlePos;
    //std::vector<glm::vec3> m_particleVel;

    Vertices m_vertices;

    VertexArrayObject	m_gpuParticleVAO;
    ArrayBuffer			m_gpuParticleBuffer;

    glm::vec4			m_wallColor{ 1, 1, 1, 0.1 };

    float m_fi = M_PI * 1.5f;
    float m_theta = M_PI / 2.0f;

    float m_u = 0.5f;
    float m_v = 0.5f;
    float step_size = 0.01f;

    glm::vec3 m_eye = glm::vec3(0, 0, 1);
    glm::vec3 m_fw = ToDescartes(m_fi, m_theta);
    glm::vec3 m_at = m_eye + m_fw;
    glm::vec3 m_up = glm::vec3(0, 1, 0);
    glm::vec3 m_left = glm::cross(m_up, m_fw);

    bool m_is_left_pressed = false;
};

