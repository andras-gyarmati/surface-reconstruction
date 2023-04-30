#include "App.h"
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
    m_virtual_camera.SetView(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
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

std::vector<vertex> application::read_vertices_from_file(std::ifstream* file, const int num_vertices)
{
    std::vector<vertex> vertices;
    vertices.resize(num_vertices);

    for (int i = 0; i < num_vertices; ++i)
    {
        *file >> vertices[i].position.x >> vertices[i].position.y >> vertices[i].position.z;
        *file >> vertices[i].color.r >> vertices[i].color.g >> vertices[i].color.b;
    }
    file->close();

    return vertices;
}

std::vector<vertex> application::load_ply_file(const std::string& filename)
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

    return read_vertices_from_file(&file, num_vertices);
}

std::vector<vertex> application::load_xyz_file(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        return {};
    }

    const int num_vertices = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');

    file.seekg(0, std::ios::beg);

    return read_vertices_from_file(&file, num_vertices);
}

bool application::init()
{
    m_virtual_camera.SetProj(glm::radians(60.0f), 640.0f / 480.0f, 0.01f, 1000.0f);

    glClearColor(0.1f, 0.1f, 0.41f, 1);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    m_axes_program.Init({{GL_VERTEX_SHADER, "axes.vert"}, {GL_FRAGMENT_SHADER, "axes.frag"}});
    m_particle_program.Init({{GL_VERTEX_SHADER, "particle.vert"}, {GL_FRAGMENT_SHADER, "particle.frag"}}, {{0, "vs_in_pos"}, {1, "vs_in_col"}, {2, "vs_in_tex"}});

    m_camera_texture.FromFile("inputs/garazs_kijarat/Dev0_Image_w960_h600_fn644.jpg");

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

    m_gpu_particle_buffer.BufferData(m_vertices);
    m_gpu_particle_vao.Init({
        {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, position)}, m_gpu_particle_buffer},
        {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, color)}, m_gpu_particle_buffer}
    });

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
    static Uint32 last_time = SDL_GetTicks();
    const float delta_time = static_cast<float>(SDL_GetTicks() - last_time) / 1000.0f;
    m_virtual_camera.Update(delta_time);
    last_time = SDL_GetTicks();
}

void application::draw_points(VertexArrayObject& vao, const size_t size)
{
    glEnable(GL_PROGRAM_POINT_SIZE);
    vao.Bind();
    m_particle_program.Use();
    m_particle_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    m_particle_program.SetUniform("world", glm::mat4(1));
    m_particle_program.SetUniform("cam_r_0", glm::vec3(m_camera_params.devices[0].r[0][0], m_camera_params.devices[0].r[0][1], m_camera_params.devices[0].r[0][2]));
    m_particle_program.SetUniform("cam_r_1", glm::vec3(m_camera_params.devices[0].r[1][0], m_camera_params.devices[0].r[1][1], m_camera_params.devices[0].r[1][2]));
    m_particle_program.SetUniform("cam_r_2", glm::vec3(m_camera_params.devices[0].r[2][0], m_camera_params.devices[0].r[2][1], m_camera_params.devices[0].r[2][2]));
    m_particle_program.SetUniform("cam_t", m_camera_params.devices[0].t);
    glm::mat3 cam_k = {
        m_camera_params.internal_params.fu, 0.0f, m_camera_params.internal_params.u0,
        0.0f, m_camera_params.internal_params.fv, m_camera_params.internal_params.v0,
        0.0f, 0.0f, 1.0f
    };
    m_particle_program.SetUniform("cam_k_0", glm::vec3(cam_k[0]));
    m_particle_program.SetUniform("cam_k_1", glm::vec3(cam_k[1]));
    m_particle_program.SetUniform("cam_k_2", glm::vec3(cam_k[2]));
    m_particle_program.SetUniform("point_size", m_point_size);
    m_particle_program.SetTexture("tex_image", 0, m_camera_texture);
    glDrawArrays(GL_POINTS, 0, size);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void application::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_axes_program.Use();
    m_axes_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    glDrawArrays(GL_LINES, 0, 6);

    draw_points(m_gpu_particle_vao, m_vertices.size());
    if (m_show_debug_sphere) draw_points(m_gpu_debug_sphere_vao, m_debug_sphere.size());


    glm::vec3 eye = m_virtual_camera.GetEye();
    glm::vec3 at = m_virtual_camera.GetAt();
    glm::vec3 up = m_virtual_camera.GetUp();
    float cam_speed = m_virtual_camera.GetSpeed();
    if (ImGui::Begin("Points"))
    {
        ImGui::Text("Properties");
        ImGui::Checkbox("Show debug sphere", &m_show_debug_sphere);
        ImGui::SliderFloat("Point size", &m_point_size, 1.0f, 10.0f);
        ImGui::SliderFloat("Cam speed", &cam_speed, 0.1f, 20.0f);
        ImGui::SliderFloat3("eye", &eye[0], -10.0f, 10.0f);
        ImGui::SliderFloat3("at", &at[0], -10.0f, 10.0f);
        ImGui::SliderFloat3("up", &up[0], -10.0f, 10.0f);
        if (ImGui::Button("up=x")) { up = glm::vec3(1, 0, 0); }
        if (ImGui::Button("up=y")) { up = glm::vec3(0, 1, 0); }
        if (ImGui::Button("up=z")) { up = glm::vec3(0, 0, 1); }
    }
    ImGui::End();
    m_virtual_camera.SetView(eye, at, up);
    m_virtual_camera.SetSpeed(cam_speed);
}

void application::keyboard_down(const SDL_KeyboardEvent& key)
{
    m_virtual_camera.KeyboardDown(key);
}

void application::keyboard_up(const SDL_KeyboardEvent& key)
{
    m_virtual_camera.KeyboardUp(key);
}

void application::mouse_move(const SDL_MouseMotionEvent& mouse)
{
    m_virtual_camera.MouseMove(mouse);
}

void application::mouse_down(const SDL_MouseButtonEvent& mouse)
{
}

void application::mouse_up(const SDL_MouseButtonEvent& mouse)
{
}

void application::mouse_wheel(const SDL_MouseWheelEvent& wheel)
{
}

void application::resize(int _w, int _h)
{
    glViewport(0, 0, _w, _h);
    m_virtual_camera.Resize(_w, _h);
}
