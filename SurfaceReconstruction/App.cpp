#include "App.h"
#include <math.h>
#include <vector>
#include <array>
#include <list>
#include <tuple>
#include <random>
#include "imgui/imgui.h"
#include "Includes/ObjParser_OGL3.h"

application::application(void)
{
    m_camera.SetView(glm::vec3(5, 5, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}

vertices application::load_ply_file(const std::string& filename) const
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
        file >> vertices.positions[i].x >> vertices.positions[i].y >> vertices.positions[i].z;
        file >> vertices.colors[i].r >> vertices.colors[i].g >> vertices.colors[i].b;
    }

    return vertices;
}

bool application::init()
{
    m_camera.SetProj(glm::radians(60.0f), 640.0f / 480.0f, 0.01f, 1000.0f);
    glClearColor(0.1f, 0.1f, 0.41f, 1);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(4.0f);
    glPointSize(15.0f);
    m_axes_program.Init({
        {GL_VERTEX_SHADER, "axes.vert"},
        {GL_FRAGMENT_SHADER, "axes.frag"}
    });
    m_particle_program.Init({{GL_VERTEX_SHADER, "particle.vert"}, {GL_FRAGMENT_SHADER, "particle.frag"}}, {{0, "vs_in_pos"}, {1, "vs_in_vel"}});
    m_vertices = load_xyz_file("../inputs/garazs_kijarat/test_fn644.xyz");
    reset();
    m_gpu_particle_buffer.BufferData(m_vertices.positions);
    m_gpu_particle_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_particle_buffer}});
    return true;
}

void application::clean()
{
}

void application::reset()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rnd(-1, 1);
}

void application::update()
{
    static Uint32 last_time = SDL_GetTicks();
    const float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;
    m_camera.Update(delta_time);
    m_gpu_particle_buffer.BufferData(m_vertices.positions);
    last_time = SDL_GetTicks();
}

void application::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_axes_program.Use();
    m_axes_program.SetUniform("mvp", m_camera.GetViewProj());

    glDrawArrays(GL_LINES, 0, 6);

    glEnable(GL_PROGRAM_POINT_SIZE);
    m_gpu_particle_vao.Bind();
    m_particle_program.Use();
    m_particle_program.SetUniform("mvp", m_camera.GetViewProj());

    glDrawArrays(GL_POINTS, 0, m_vertices.positions.size());

    glDisable(GL_PROGRAM_POINT_SIZE);

    if (ImGui::Begin("Points"))
    {
        ImGui::Text("Properties");
    }
    ImGui::End();
}

glm::vec3 application::to_descartes(const float fi, const float theta)
{
    return {
        sinf(theta) * cosf(fi),
        cosf(theta),
        sinf(theta) * sinf(fi)
    };
}

void application::keyboard_down(const SDL_KeyboardEvent& key)
{
    m_camera.KeyboardDown(key);
}

void application::keyboard_up(const SDL_KeyboardEvent& key)
{
    m_camera.KeyboardUp(key);
}

void application::mouse_move(const SDL_MouseMotionEvent& mouse)
{
    m_camera.MouseMove(mouse);
}

void application::mouse_down(const SDL_MouseButtonEvent& mouse)
{
}

void application::mouse_up(const SDL_MouseButtonEvent& mouse)
{
}

void application::mouse_wheel(SDL_MouseWheelEvent& wheel)
{
}

void application::resize(int _w, int _h)
{
    glViewport(0, 0, _w, _h);
    m_camera.Resize(_w, _h);
}
