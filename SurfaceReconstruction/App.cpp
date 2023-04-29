﻿#include "App.h"
#include <math.h>
#include <vector>
#include <array>
#include <list>
#include <tuple>
#include <random>
#include "imgui/imgui.h"
#include "Includes/ObjParser_OGL3.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <limits>
#include <algorithm>
#include <set>
#include <unordered_set>

application::application(void)
{
    m_camera.SetView(glm::vec3(0, 0, 0), glm::vec3(5, 0, 5), glm::vec3(0, 1, 0));
}

camera_params application::load_camera_params(const std::string& filename)
{
    camera_params camera_params;

    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        return camera_params;
    }

    file >> camera_params.internal_params.fu
        >> camera_params.internal_params.u0
        >> camera_params.internal_params.fv
        >> camera_params.internal_params.v0;

    int num_devices;
    file >> num_devices;

    for (int i = 0; i < num_devices; ++i)
    {
        device dev;

        file >> dev.name;

        glm::mat3& r = dev.r;
        for (int j = 0; j < 3; ++j)
        {
            for (int k = 0; k < 3; ++k)
            {
                file >> r[j][k];
            }
        }

        for (int j = 0; j < 3; ++j)
        {
            float t_val;
            file >> t_val;
            dev.t[j] = t_val;
        }

        camera_params.devices.push_back(dev);
    }

    return camera_params;
}

glm::vec3 application::to_descartes(const float fi, const float theta)
{
    return {
        sinf(theta) * cosf(fi),
        cosf(theta),
        sinf(theta) * sinf(fi)
    };
}

glm::vec3 application::get_sphere_pos(const float u, const float v)
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

vertices application::load_ply_file(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        return {};
    }

    std::string line;
    int num_vertices = 0;
    bool header_finished = false;
    while (std::getline(file, line))
    {
        if (line == "end_header")
        {
            header_finished = true;
            break;
        }

        if (line.find("element vertex") == 0)
        {
            std::sscanf(line.c_str(), "element vertex %d", &num_vertices);
        }
    }

    if (!header_finished)
    {
        std::cerr << "Error: Could not find end_header in PLY file" << std::endl;
        return {};
    }

    vertices vertices;
    vertices.positions.resize(num_vertices);
    vertices.colors.resize(num_vertices);
    for (int i = 0; i < num_vertices; ++i)
    {
        file >> vertices.positions[i].x >> vertices.positions[i].y >> vertices.positions[i].z;
        file >> vertices.colors[i].r >> vertices.colors[i].g >> vertices.colors[i].b;
    }

    return vertices;
}

vertices application::load_xyz_file(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        return {};
    }

    const int num_vertices = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');

    file.seekg(0, std::ios::beg);

    vertices vertices;
    vertices.positions.resize(num_vertices);
    vertices.colors.resize(num_vertices);

    for (int i = 0; i < num_vertices; ++i)
    {
        file >> vertices.positions[i].x >> vertices.positions[i].y >> vertices.positions[i].z; // todo
        file >> vertices.colors[i].r >> vertices.colors[i].g >> vertices.colors[i].b;
    }

    return vertices;
}

bool application::init()
{
    m_mat_proj = glm::perspective(60.0f, 640.0f / 480.0f, 1.0f, 1000.0f);
    m_camera.SetProj(glm::radians(60.0f), 640.0f / 480.0f, 0.01f, 1000.0f);

    glClearColor(0.1f, 0.1f, 0.41f, 1);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    m_axes_program.Init({{GL_VERTEX_SHADER, "axes.vert"}, {GL_FRAGMENT_SHADER, "axes.frag"}});
    m_particle_program.Init({{GL_VERTEX_SHADER, "particle.vert"}, {GL_FRAGMENT_SHADER, "particle.frag"}}, {{0, "vs_in_pos"}, {1, "vs_in_tex"}});

    m_camera_texture.FromFile("inputs/garazs_kijarat/Dev0_Image_w960_h600_fn644.jpg");
    // m_camera_texture.FromFile("inputs/debug.png");

    m_camera_params = load_camera_params("inputs/CameraParameters_minimal.txt");
    m_vertices = load_xyz_file("inputs/garazs_kijarat/test_fn644.xyz");

    reset();

    constexpr int n = 960;
    constexpr int m = 960;
    m_debug_sphere.resize((m + 1) * (n + 1));
    for (int i = 0; i <= n; ++i)
        for (int j = 0; j <= m; ++j)
            m_debug_sphere[i + j * (n + 1)] = get_sphere_pos(static_cast<float>(i) / static_cast<float>(n), static_cast<float>(j) / static_cast<float>(m));
    m_gpu_debug_sphere_buffer.BufferData(m_debug_sphere);
    m_gpu_debug_sphere_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_debug_sphere_buffer}});

    m_gpu_particle_buffer.BufferData(m_vertices.positions);
    m_gpu_particle_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_particle_buffer}});

    return true;
}

void application::clean()
{
}

void application::reset()
{
}

void application::update()
{
    m_mat_view = glm::lookAt(m_eye, m_at, m_up);
    static Uint32 last_time = SDL_GetTicks();
    const float delta_time = static_cast<float>(SDL_GetTicks() - last_time) / 1000.0f;
    m_camera.Update(delta_time);
    m_gpu_particle_buffer.BufferData(m_vertices.positions);
    last_time = SDL_GetTicks();
}

void application::draw_points(glm::mat4 mvp, glm::mat4 world, VertexArrayObject& vao, ProgramObject& program, const size_t size, camera_params cam_params, Texture2D& texture)
{
    glEnable(GL_PROGRAM_POINT_SIZE);
    vao.Bind();
    program.Use();
    program.SetUniform("mvp", mvp);
    program.SetUniform("world", world);
    program.SetUniform("cam_r_0", glm::vec3(cam_params.devices[0].r[0][0], cam_params.devices[0].r[0][1], cam_params.devices[0].r[0][2]));
    program.SetUniform("cam_r_1", glm::vec3(cam_params.devices[0].r[1][0], cam_params.devices[0].r[1][1], cam_params.devices[0].r[1][2]));
    program.SetUniform("cam_r_2", glm::vec3(cam_params.devices[0].r[2][0], cam_params.devices[0].r[2][1], cam_params.devices[0].r[2][2]));
    program.SetUniform("cam_t", cam_params.devices[0].t);
    glm::mat3 cam_k = {
        cam_params.internal_params.fu, 0.0f, cam_params.internal_params.u0,
        0.0f, cam_params.internal_params.fv, cam_params.internal_params.v0,
        0.0f, 0.0f, 1.0f
    };
    //cam_k = glm::inverse(cam_k);
    program.SetUniform("cam_k_0", glm::vec3(cam_k[0]));
    program.SetUniform("cam_k_1", glm::vec3(cam_k[1]));
    program.SetUniform("cam_k_2", glm::vec3(cam_k[2]));
    program.SetTexture("tex_image", 0, texture);
    glDrawArrays(GL_POINTS, 0, size);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void application::render()
{
    const glm::mat4 rot_x_neg_90 = glm::rotate<float>(static_cast<float>(-(M_PI / 2)), glm::vec3(1, 0, 0));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glm::mat4 mvp = m_camera.GetViewProj();
    glm::mat4 mvp = m_mat_proj * m_mat_view * m_mat_world;
    mvp *= rot_x_neg_90;

    // glm::mat4 world = m_camera.GetProj();
    glm::mat4 world = m_mat_world;
    world *= rot_x_neg_90;

    m_axes_program.Use();
    m_axes_program.SetUniform("mvp", mvp);
    glDrawArrays(GL_LINES, 0, 6);

    draw_points(mvp, world, m_gpu_particle_vao, m_particle_program, m_vertices.positions.size(), m_camera_params, m_camera_texture);
    draw_points(mvp, world, m_gpu_debug_sphere_vao, m_particle_program, m_debug_sphere.size(), m_camera_params, m_camera_texture);

    if (ImGui::Begin("Points"))
    {
        ImGui::Text("Properties");
    }
    ImGui::End();
}

void application::keyboard_down(const SDL_KeyboardEvent& key)
{
    m_camera.KeyboardDown(key);
    switch (key.keysym.sym)
    {
    case SDLK_w: std::cout << "---\n|W|\n";
        m_eye += m_fw * m_speed;
        m_at += m_fw * m_speed;
        break;
    case SDLK_s: std::cout << "---\n|S|\n";
        m_eye -= m_fw * m_speed;
        m_at -= m_fw * m_speed;
        break;
    case SDLK_d: std::cout << "---\n|D|\n";
        m_eye -= m_left * m_speed;
        m_at -= m_left * m_speed;
        break;
    case SDLK_a: std::cout << "---\n|A|\n";
        m_eye += m_left * m_speed;
        m_at += m_left * m_speed;
        break;
    default: break;
    }
}

void application::keyboard_up(const SDL_KeyboardEvent& key)
{
    m_camera.KeyboardUp(key);
}

void application::mouse_move(const SDL_MouseMotionEvent& mouse)
{
    m_camera.MouseMove(mouse);
    if (m_is_left_pressed)
    {
        m_fi += static_cast<float>(mouse.xrel) / 100.0f;
        m_theta += static_cast<float>(mouse.yrel) / 100.0f;
        m_theta = glm::clamp(m_theta, 0.1f, 3.1f);
        m_fw = to_descartes(m_fi, m_theta);
        m_eye = m_at - m_fw;
        m_left = glm::cross(m_up, m_fw);
    }
}

void application::mouse_down(const SDL_MouseButtonEvent& mouse)
{
    if (mouse.button == SDL_BUTTON_LEFT)
        m_is_left_pressed = true;
}

void application::mouse_up(const SDL_MouseButtonEvent& mouse)
{
    if (mouse.button == SDL_BUTTON_LEFT)
        m_is_left_pressed = false;
}

void application::mouse_wheel(const SDL_MouseWheelEvent& wheel)
{
}

void application::resize(int _w, int _h)
{
    glViewport(0, 0, _w, _h);
    m_mat_proj = glm::perspective(glm::radians(60.0f), static_cast<float>(_w) / static_cast<float>(_h), 0.01f, 1000.0f);
    m_camera.Resize(_w, _h);
}
