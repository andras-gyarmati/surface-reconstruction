#include <iostream>
#include "gCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>

/// <summary>
/// Initializes a new instance of the <see cref="gCamera"/> class.
/// </summary>
gCamera::gCamera(void) : m_speed(4.0f), m_fast(false), m_eye(0.0f, 20.0f, 20.0f), m_up(0.0f, 1.0f, 0.0f), m_at(0.0f), m_goFw(0), m_goRight(0), m_goUp(0) {
    SetView(glm::vec3(0, 20, 20), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    m_dist = length(m_at - m_eye);

    SetProj(glm::radians(60.0f), 640 / 480.0f, 0.01f, 1000.0f);
}

gCamera::gCamera(glm::vec3 _eye, glm::vec3 _at, glm::vec3 _up) : m_speed(4.0f), m_goFw(0), m_goRight(0), m_dist(10), m_goUp(0), m_fast(false) {
    SetView(_eye, _at, _up);
}

gCamera::~gCamera(void) {}

void gCamera::SetView(glm::vec3 _eye, glm::vec3 _at, glm::vec3 _up) {
    m_eye = _eye;
    m_at = _at;
    m_up = _up;

    m_fw = normalize(m_at - m_eye);
    m_st = normalize(cross(m_fw, m_up));

    m_dist = length(m_at - m_eye);

    m_u = atan2f(m_fw.z, m_fw.x);
    m_v = acosf(m_fw.y);
}

void gCamera::SetProj(float _angle, float _aspect, float _zn, float _zf) {
    m_matProj = glm::perspective(_angle, _aspect, _zn, _zf);
    m_matViewProj = m_matProj * m_viewMatrix;
}

glm::mat4 gCamera::GetViewMatrix() const {
    return m_viewMatrix;
}

void gCamera::Update(float _deltaTime) {
    m_eye += (m_goFw * m_fw + m_goRight * m_st + m_goUp * m_up) * m_speed * _deltaTime;
    m_at += (m_goFw * m_fw + m_goRight * m_st + m_goUp * m_up) * m_speed * _deltaTime;

    m_viewMatrix = lookAt(m_eye, m_at, m_up);
    m_matViewProj = m_matProj * m_viewMatrix;
}

void gCamera::UpdateUV(float du, float dv) {
    // Update m_u and m_v
    m_u += du;
    m_v = glm::clamp<float>(m_v + dv, 0.1f, 3.1f);

    // Calculate the rotation axis for vertical movement
    glm::vec3 rotation_axis = cross(m_fw, m_up);

    // Create horizontal and vertical rotation matrices
    glm::mat4 horizontal_rotation = glm::rotate(glm::mat4(1.0f), -du, m_up);
    glm::mat4 vertical_rotation = glm::rotate(glm::mat4(1.0f), -dv, rotation_axis);

    // Combine the horizontal and vertical rotations
    glm::mat4 combined_rotation = horizontal_rotation * vertical_rotation;

    // Apply the rotation to the current forward direction
    glm::vec3 new_fw = glm::vec3(combined_rotation * glm::vec4(m_fw, 0.0f));

    // Update m_at and m_fw
    m_at = m_eye + m_dist * new_fw;
    m_fw = normalize(new_fw);

    // Recalculate m_st
    m_st = normalize(cross(m_fw, m_up));
}

void gCamera::SetSpeed(float _val) {
    m_speed = _val;
}

void gCamera::Resize(int _w, int _h) {
    SetProj(glm::radians(60.0f), _w / (float)_h, 0.01f, 1000.0f);
}

void gCamera::KeyboardDown(const SDL_KeyboardEvent& key) {
    switch (key.keysym.sym) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        if (!m_fast) {
            m_fast = true;
            m_speed *= 4.0f;
        }
        break;
    case SDLK_w:
        m_goFw = 1;
        break;
    case SDLK_s:
        m_goFw = -1;
        break;
    case SDLK_a:
        m_goRight = -1;
        break;
    case SDLK_d:
        m_goRight = 1;
        break;
    case SDLK_q:
        m_goUp = -1;
        break;
    case SDLK_e:
        m_goUp = 1;
        break;
    }
}

void gCamera::KeyboardUp(const SDL_KeyboardEvent& key) {
    float current_speed = m_speed;
    switch (key.keysym.sym) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        if (m_fast) {
            m_fast = false;
            m_speed /= 4.0f;
        }
        break;
    case SDLK_w:
    case SDLK_s:
        m_goFw = 0;
        break;
    case SDLK_a:
    case SDLK_d:
        m_goRight = 0;
        break;
    case SDLK_q:
    case SDLK_e:
        m_goUp = 0;
        break;
    }
}

void gCamera::MouseMove(const SDL_MouseMotionEvent& mouse) {
    if (mouse.state & SDL_BUTTON_LMASK) {
        UpdateUV(mouse.xrel / 100.0f, mouse.yrel / 100.0f);
    }
}

void gCamera::LookAt(glm::vec3 _at) {
    SetView(m_eye, _at, m_up);
}
