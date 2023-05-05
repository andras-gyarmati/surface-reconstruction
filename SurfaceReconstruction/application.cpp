#include <math.h>
#include <vector>
#include <random>
#include <glm/glm.hpp>
#include "application.h"
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include "window_utils.h"
#include "file_loader.h"

application::application(void) {
    m_virtual_camera.SetView(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
    strncpy_s(m_input_folder, "inputs/elte_logo", sizeof(m_input_folder));
    m_input_folder[sizeof(m_input_folder) - 1] = '\0';
}

void application::load_inputs_from_folder(const std::string& folder_name) {
    const std::vector<std::string> file_paths = file_loader::get_directory_files(folder_name);
    std::string xyz_file;

    for (const auto& file : file_paths) {
        if (file.find("Dev0") != std::string::npos) {
            m_digital_camera_textures[0].FromFile(file);
        }
        else if (file.find("Dev1") != std::string::npos) {
            m_digital_camera_textures[1].FromFile(file);
        }
        else if (file.find("Dev2") != std::string::npos) {
            m_digital_camera_textures[2].FromFile(file);
        }
        else if (file.find(".xyz") != std::string::npos) {
            xyz_file = file;
        }
    }

    m_vertices = file_loader::load_xyz_file(xyz_file);
    m_digital_camera_params = file_loader::load_digital_camera_params("inputs/CameraParameters_minimal.txt");

    m_gpu_particle_buffer.BufferData(m_vertices);
    m_gpu_particle_vao.Init({
        {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_gpu_particle_buffer},
        {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_gpu_particle_buffer}
    });


}

void application::init_debug_sphere() {
    constexpr int n = 960;
    constexpr int m = 960;
    m_debug_sphere.resize((m + 1) * (n + 1));
    for (int i = 0; i <= n; ++i)
        for (int j = 0; j <= m; ++j)
            m_debug_sphere[i + j * (n + 1)] = get_sphere_pos(static_cast<float>(i) / static_cast<float>(n), static_cast<float>(j) / static_cast<float>(m));
    m_gpu_debug_sphere_buffer.BufferData(m_debug_sphere);
    m_gpu_debug_sphere_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_debug_sphere_buffer}});
}

void application::init_box(const glm::vec3& top_left_front, const glm::vec3& bottom_right_back) {
    glm::vec3 bottom_left_front(top_left_front.x, bottom_right_back.y, top_left_front.z);
    glm::vec3 top_right_back(bottom_right_back.x, top_left_front.y, bottom_right_back.z);
    glm::vec3 bottom_left_back(top_left_front.x, bottom_right_back.y, bottom_right_back.z);
    glm::vec3 top_right_front(bottom_right_back.x, top_left_front.y, top_left_front.z);
    glm::vec3 top_left_back(top_left_front.x, top_left_front.y, bottom_right_back.z);
    glm::vec3 bottom_right_front(bottom_right_back.x, bottom_right_back.y, top_left_front.z);

    m_box_gpu_buffer_pos.BufferData(
        std::vector<glm::vec3>{
            // back face
            bottom_left_back,
            bottom_right_back,
            top_right_back,
            top_left_back,
            // front face
            bottom_left_front,
            bottom_right_front,
            top_right_front,
            top_left_front,
        }
    );

    m_box_gpu_buffer_indices.BufferData(
        std::vector<int>{
            // back face
            0, 1, 1, 2, 2, 3, 3, 0,
            // front face
            4, 5, 5, 6, 6, 7, 7, 4,
            // connecting edges
            0, 4, 1, 5, 2, 6, 3, 7,
        }
    );

    m_box_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_box_gpu_buffer_pos},}, m_box_gpu_buffer_indices);
}

bool application::init(SDL_Window* window) {
    m_window = window;
    m_virtual_camera.SetProj(glm::radians(60.0f), 1280.0f / 720.0f, 0.01f, 1000.0f);

    glClearColor(0.1f, 0.1f, 0.41f, 1);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    m_axes_program.Init({{GL_VERTEX_SHADER, "axes.vert"}, {GL_FRAGMENT_SHADER, "axes.frag"}});
    m_particle_program.Init({{GL_VERTEX_SHADER, "particle.vert"}, {GL_FRAGMENT_SHADER, "particle.frag"}}, {{0, "vs_in_pos"}, {1, "vs_in_col"}, {2, "vs_in_tex"}});
    m_box_wireframe_program.Init({{GL_VERTEX_SHADER, "box_wireframe.vert"}, {GL_FRAGMENT_SHADER, "box_wireframe.frag"}}, {{0, "vs_in_pos"}});

    load_inputs_from_folder("inputs/garazs_kijarat");
    init_debug_sphere();

    return true;
}

void application::clean() {}

void application::reset() {}

void application::update() {
    static Uint32 last_time = SDL_GetTicks();
    const float delta_time = static_cast<float>(SDL_GetTicks() - last_time) / 1000.0f;
    m_virtual_camera.Update(delta_time);
    last_time = SDL_GetTicks();
}

void application::draw_points(VertexArrayObject& vao, const size_t size) {
    glEnable(GL_PROGRAM_POINT_SIZE);
    vao.Bind();
    m_particle_program.Use();
    m_particle_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    m_particle_program.SetUniform("world", glm::mat4(1));
    m_particle_program.SetUniform("point_size", m_point_size);

    m_particle_program.SetUniform("cam_k", m_digital_camera_params.get_cam_k());

    m_particle_program.SetUniform("cam_r[0]", m_digital_camera_params.devices[0].r);
    m_particle_program.SetUniform("cam_r[1]", m_digital_camera_params.devices[1].r);
    m_particle_program.SetUniform("cam_r[2]", m_digital_camera_params.devices[2].r);

    m_particle_program.SetUniform("cam_t[0]", m_digital_camera_params.devices[0].t);
    m_particle_program.SetUniform("cam_t[1]", m_digital_camera_params.devices[1].t);
    m_particle_program.SetUniform("cam_t[2]", m_digital_camera_params.devices[2].t);

    m_particle_program.SetTexture("tex_image[0]", 0, m_digital_camera_textures[0]);
    m_particle_program.SetTexture("tex_image[1]", 1, m_digital_camera_textures[1]);
    m_particle_program.SetTexture("tex_image[2]", 2, m_digital_camera_textures[2]);

    glDrawArrays(GL_POINTS, 0, size);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

void application::render_imgui() {
    glm::vec3 eye = m_virtual_camera.GetEye();
    glm::vec3 at = m_virtual_camera.GetAt();
    glm::vec3 up = m_virtual_camera.GetUp();
    float cam_speed = m_virtual_camera.GetSpeed();
    if (ImGui::Begin("Points")) {
        if (ImGui::Button("Toggle Fullscreen")) {
            toggle_fullscreen(m_window);
        }
        if (ImGui::Button("load garazs_kijarat")) {
            load_inputs_from_folder("inputs/garazs_kijarat");
        }
        if (ImGui::Button("load elte_logo")) {
            load_inputs_from_folder("inputs/elte_logo");
        }
        if (ImGui::Button("load parkolo_gomb")) {
            load_inputs_from_folder("inputs/parkolo_gomb");
        }
        ImGui::InputText("Folder path", m_input_folder, sizeof(m_input_folder));
        if (ImGui::Button("Load points")) {
            load_inputs_from_folder(m_input_folder);
        }
        ImGui::Text("Properties");
        ImGui::Checkbox("Show debug sphere", &m_show_debug_sphere);
        ImGui::SliderFloat("Point size", &m_point_size, 1.0f, 30.0f);
        ImGui::SliderFloat("Cam speed", &cam_speed, 0.1f, 20.0f);
        ImGui::SliderFloat3("eye", &eye[0], -10.0f, 10.0f);
        ImGui::SliderFloat3("at", &at[0], -1.0f, 1.0f);
        ImGui::SliderFloat3("up", &up[0], -1.0f, 1.0f);
        ImGui::SliderFloat3("m_top_left_front", &m_top_left_front[0], -10.0f, 10.0f);
        ImGui::SliderFloat3("m_bottom_right_back", &m_bottom_right_back[0], -10.0f, 10.0f);
        if (ImGui::Button("reset camera")) {
            eye = glm::vec3(0, 0, 0);
            at = glm::vec3(0, 1, 0);
            up = glm::vec3(0, 0, 1);
        }
    }
    ImGui::End();
    m_virtual_camera.SetView(eye, at, up);
    m_virtual_camera.SetSpeed(cam_speed);
}

void application::render_box() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    m_box_vao.Bind();
    m_box_wireframe_program.Use();
    m_box_wireframe_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
}

void application::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_axes_program.Use();
    m_axes_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    glDrawArrays(GL_LINES, 0, 6);

    draw_points(m_gpu_particle_vao, m_vertices.size());
    if (m_show_debug_sphere) draw_points(m_gpu_debug_sphere_vao, m_debug_sphere.size());

    init_box(m_top_left_front, m_bottom_right_back);
    render_box();

    render_imgui();
}

void application::keyboard_down(const SDL_KeyboardEvent& key) {
    m_virtual_camera.KeyboardDown(key);
}

void application::keyboard_up(const SDL_KeyboardEvent& key) {
    m_virtual_camera.KeyboardUp(key);
}

void application::mouse_move(const SDL_MouseMotionEvent& mouse) {
    m_virtual_camera.MouseMove(mouse);
}

void application::mouse_down(const SDL_MouseButtonEvent& mouse) {}

void application::mouse_up(const SDL_MouseButtonEvent& mouse) {}

void application::mouse_wheel(const SDL_MouseWheelEvent& wheel) {}

void application::resize(int _w, int _h) {
    glViewport(0, 0, _w, _h);
    m_virtual_camera.Resize(_w, _h);
}
