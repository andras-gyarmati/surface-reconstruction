#include "MyApp.h"
#include <math.h>
#include <vector>
#include <array>
#include <list>
#include <tuple>
#include <random>
#include "imgui/imgui.h"
#include "Includes/ObjParser_OGL3.h"

Application::Application(void)
{
    // m_camera.SetView(glm::vec3(5, 5, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}

Application::~Application(void)
= default;

Vertices Application::load_ply_file(const std::string& filename) const
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        return {};
    }

    std::string line;
    int numVertices = 0;
    bool headerFinished = false;
    while (std::getline(file, line))
    {
        if (line == "end_header")
        {
            headerFinished = true;
            break;
        }

        if (line.find("element vertex") == 0)
        {
            std::sscanf(line.c_str(), "element vertex %d", &numVertices);
        }
    }

    if (!headerFinished)
    {
        std::cerr << "Error: Could not find end_header in PLY file" << std::endl;
        return {};
    }

    Vertices vertices;
    vertices.positions.resize(numVertices);
    vertices.colors.resize(numVertices);
    for (int i = 0; i < numVertices; ++i)
    {
        file >> vertices.positions[i].x >> vertices.positions[i].y >> vertices.positions[i].z;
        file >> vertices.colors[i].r >> vertices.colors[i].g >> vertices.colors[i].b;
    }

    return vertices;
}

Vertices Application::load_xyz_file(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        return {};
    }

    const int num_vertices = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');

    file.seekg(0, std::ios::beg);

    Vertices vertices;
    vertices.positions.resize(num_vertices);
    vertices.colors.resize(num_vertices);

    for (int i = 0; i < num_vertices; ++i)
    {
        file >> vertices.positions[i].x >> vertices.positions[i].y >> vertices.positions[i].z;
        file >> vertices.colors[i].r >> vertices.colors[i].g >> vertices.colors[i].b;
    }

    return vertices;
}

bool Application::init()
{
    m_mat_proj = glm::perspective(45.0f, 640 / 480.0f, 1.0f, 1000.0f);
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

    // m_program.Init(
    //     // shaderek felsorolása
    //     {
    //         {GL_VERTEX_SHADER, "myVert.vert"},
    //         {GL_FRAGMENT_SHADER, "myFrag.frag"}
    //     },
    //     // binding-ok felsorolása
    //     {
    //         {0, "vs_in_pos"}, // VAO 0-as csatorna menjen a vs_in_pos-ba
    //         {1, "vs_in_normal"}, // VAO 1-es csatorna menjen a vs_in_normal-ba
    //     });

    // geometria VAO-ban való regisztrálása
    // m_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_buffer_pos},}, m_gpu_buffer_indices);

    // kamera
    // m_camera.SetProj(glm::radians(60.0f), 640.0f / 480.0f, 0.01f, 1000.0f);

    // részecskék inicializálása

    // m_vertices = loadPLYFile("../inputs/sphereCalibScan5.ply");
    m_vertices = load_xyz_file("../inputs/garazs_kijarat/test_fn644.xyz");

    // véletlenszám generátor inicializálása
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rnd(-1, 1);

    // GPU-ra áttölteni a részecskék pozícióit
    m_gpu_particle_buffer.BufferData(m_vertices.positions);

    // és végül a VAO-t inicializálni
    m_gpu_particle_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_particle_buffer}});

    return true;
}

void Application::clean()
{
}

void Application::reset()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rnd(-1, 1);
}

void Application::update()
{
    m_mat_view = glm::lookAt(m_eye, m_at, m_up);
    // static Uint32 last_time = SDL_GetTicks();
    // float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;
    // m_camera.Update(delta_time);
    m_gpu_particle_buffer.BufferData(m_vertices.positions);
    // last_time = SDL_GetTicks();
}

void Application::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 mvp = m_mat_proj * m_mat_view * m_mat_world;

    // tengelyek
    m_axes_program.Use();
    m_axes_program.SetUniform("mvp", mvp);

    glDrawArrays(GL_LINES, 0, 6);

    glEnable(GL_PROGRAM_POINT_SIZE);
    m_gpu_particle_vao.Bind();
    m_particle_program.Use();
    m_particle_program.SetUniform("mvp", mvp);

    glDrawArrays(GL_POINTS, 0, m_vertices.positions.size());

    glDisable(GL_PROGRAM_POINT_SIZE);

    if (ImGui::Begin("Points"))
    {
        ImGui::Text("Properties");
    }
    ImGui::End();
}

glm::vec3 Application::to_descartes(const float fi, const float theta)
{
    return {
        sinf(theta) * cosf(fi),
        cosf(theta),
        sinf(theta) * sinf(fi)
    };
}

void Application::keyboard_down(const SDL_KeyboardEvent& key)
{
    // m_camera.KeyboardDown(key);
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

void Application::keyboard_up(SDL_KeyboardEvent& key)
{
    // m_camera.KeyboardUp(key);
}

void Application::mouse_move(const SDL_MouseMotionEvent& mouse)
{
    // m_camera.MouseMove(mouse);
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

void Application::mouse_down(const SDL_MouseButtonEvent& mouse)
{
    if (mouse.button == SDL_BUTTON_LEFT)
        m_is_left_pressed = true;
}

void Application::mouse_up(const SDL_MouseButtonEvent& mouse)
{
    if (mouse.button == SDL_BUTTON_LEFT)
        m_is_left_pressed = false;
}

void Application::mouse_wheel(SDL_MouseWheelEvent& wheel)
{
}

void Application::resize(int _w, int _h)
{
    glViewport(0, 0, _w, _h);
    // m_camera.Resize(_w, _h);
    m_mat_proj = glm::perspective(glm::radians(60.0f), static_cast<float>(_w) / static_cast<float>(_h), 0.01f, 1000.0f);
}
