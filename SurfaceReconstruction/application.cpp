#include <math.h>
#include <vector>
#include <stack>
#include <random>
#include <glm/glm.hpp>
#include "application.h"
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include "window_utils.h"
#include "file_loader.h"

application::application(void) {
    m_start_eye = glm::vec3(0, 0, 0);
    m_start_at = glm::vec3(1, 0, 0);
    m_start_up = glm::vec3(0, 0, 1);
    m_virtual_camera.SetView(m_start_eye, m_start_at, m_start_up);
    strncpy_s(m_input_folder, "inputs/elte_logo", sizeof(m_input_folder));
    m_input_folder[sizeof(m_input_folder) - 1] = '\0';
    m_point_size = 4.f;
    m_show_debug_sphere = false;
    m_show_octree = false;
    m_auto_increment_rendered_point_index = false;
    m_render_points_up_to_index = 192;
    m_ignore_center_radius = 1.3f;
    m_mesh_rendering_mode = solid;
}

void application::init_octree() {
    m_octree = octree(glm::vec3(-100, -100, -100), glm::vec3(100, 100, 100));
    m_points_to_add_index = -1;
    m_points_added_index = -1;
    m_box_indices = {};
    m_box_pos = {};
}

void application::load_inputs_from_folder(const std::string& folder_name) {
    const std::vector<std::string> file_paths = file_loader::get_directory_files(folder_name);
    std::string xyz_file;

    for (const auto& file : file_paths) {
        if (file.find("Dev0") != std::string::npos) {
            m_digital_camera_textures[0].FromFile(file);
        } else if (file.find("Dev1") != std::string::npos) {
            m_digital_camera_textures[1].FromFile(file);
        } else if (file.find("Dev2") != std::string::npos) {
            m_digital_camera_textures[2].FromFile(file);
        } else if (file.find(".xyz") != std::string::npos) {
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

    randomize_colors();

    init_octree();

    for (int i = 0; i < m_vertices.size(); ++i) {
        m_octree.insert(m_vertices[i].position);
    }

    init_octree_visualization(&m_octree);
    init_mesh_visualization();
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

    auto pos = std::vector<glm::vec3>{
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
    };
    m_box_pos.insert(m_box_pos.end(), pos.begin(), pos.end());

    auto indices = std::vector<int>{
        // back face
        0, 1, 1, 2, 2, 3, 3, 0,
        // front face
        4, 5, 5, 6, 6, 7, 7, 4,
        // connecting edges
        0, 4, 1, 5, 2, 6, 3, 7,
    };
    for (auto& index : indices) {
        index += m_box_indices.size();
    }
    m_box_indices.insert(m_box_indices.end(), indices.begin(), indices.end());
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
    m_mesh_program.Init({{GL_VERTEX_SHADER, "mesh.vert"}, {GL_FRAGMENT_SHADER, "mesh.frag"}}, {{0, "vs_in_pos"}, {1, "vs_in_col"}, {2, "vs_in_tex"}});

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

    if (m_auto_increment_rendered_point_index) {
        m_render_points_up_to_index += 1;
    }

    if (m_render_points_up_to_index > m_vertices.size()) {
        m_render_points_up_to_index = m_vertices.size();
    }
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
    if (ImGui::Begin("settings")) {
        if (ImGui::Button("toggle Fullscreen")) {
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
        ImGui::InputText("folder path", m_input_folder, sizeof(m_input_folder));
        if (ImGui::Button("Load points")) {
            load_inputs_from_folder(m_input_folder);
        }
        ImGui::Text("properties");
        ImGui::Checkbox("show debug sphere", &m_show_debug_sphere);
        ImGui::Checkbox("show octree", &m_show_octree);
        ImGui::Checkbox("auto increment rendered point index", &m_auto_increment_rendered_point_index);
        ImGui::SliderInt("points index", &m_render_points_up_to_index, 0, m_vertices.size());
        if (ImGui::Button("+1")) {
            ++m_render_points_up_to_index;
        }
        if (ImGui::Button("-1")) {
            --m_render_points_up_to_index;
        }
        ImGui::Text("mesh rendering mode");
        ImGui::SliderInt(m_mesh_rendering_mode == none ? "none" : m_mesh_rendering_mode == wireframe ? "wireframe" : "solid", &m_mesh_rendering_mode, none, solid);
        ImGui::SliderFloat("point size", &m_point_size, 1.0f, 30.0f);
        ImGui::SliderFloat("cam speed", &cam_speed, 0.1f, 20.0f);
        ImGui::SliderFloat("ignore center radius", &m_ignore_center_radius, 0.1f, 4.0f);
        if (ImGui::Button("reset camera")) {
            eye = m_start_eye;
            at = m_start_at;
            up = m_start_up;
        }
        if (ImGui::Button("set camera far")) {
            eye = glm::vec3(100, 100, 100);
            at = glm::vec3(0, 0, 0);
            up = glm::vec3(0, 0, 1);
        }
    }
    ImGui::End();
    m_virtual_camera.SetView(eye, at, up);
    m_virtual_camera.SetSpeed(cam_speed);
}

void application::render_octree_boxes() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    m_box_vao.Bind();
    m_box_wireframe_program.Use();
    m_box_wireframe_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    glDrawElements(GL_LINES, m_box_indices.size(), GL_UNSIGNED_INT, nullptr);
}

void application::init_octree_visualization(const octree* root) {
    if (!root)
        return;

    std::stack<const octree*> octree_stack;
    octree_stack.push(root);

    while (!octree_stack.empty()) {
        const octree* node = octree_stack.top();
        octree_stack.pop();

        if (!node->m_children.empty()) {
            for (const auto child : node->m_children) {
                if (child != nullptr) {
                    octree_stack.push(child);
                }
            }
        }

        if (node->m_top_left_front != nullptr) {
            init_box(*node->m_top_left_front, *node->m_bottom_right_back);
        }
    }

    m_box_pos_gpu_buffer.BufferData(m_box_pos);
    m_box_indices_gpu_buffer.BufferData(m_box_indices);
    m_box_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_box_pos_gpu_buffer}}, m_box_indices_gpu_buffer);
}

void application::init_mesh_visualization() {
    m_mesh_vertices = m_vertices; //
    m_mesh_indices.clear();
    // we use a [/] quad right now but if the top right vertex is missing we lose two triangles
    // so we need to check if a vertex is missing we might can use a [\] quad there and have more triangles
    for (int i = 0; i < m_render_points_up_to_index; ++i) {
        if (!((i % (16 * 12)) > (14 * 12 - 1) || ((i % 12) == 11))) {
            if (glm::distance(m_mesh_vertices[i].position, glm::vec3(0, 0, 0)) > m_ignore_center_radius &&
                glm::distance(m_mesh_vertices[i + 1].position, glm::vec3(0, 0, 0)) > m_ignore_center_radius &&
                glm::distance(m_mesh_vertices[i + 25].position, glm::vec3(0, 0, 0)) > m_ignore_center_radius) {
                m_mesh_indices.push_back(i + 0);
                m_mesh_indices.push_back(i + 1);
                m_mesh_indices.push_back(i + 25);
            }
            if (glm::distance(m_mesh_vertices[i].position, glm::vec3(0, 0, 0)) > m_ignore_center_radius &&
                glm::distance(m_mesh_vertices[i + 24].position, glm::vec3(0, 0, 0)) > m_ignore_center_radius &&
                glm::distance(m_mesh_vertices[i + 25].position, glm::vec3(0, 0, 0)) > m_ignore_center_radius) {
                m_mesh_indices.push_back(i + 0);
                m_mesh_indices.push_back(i + 25);
                m_mesh_indices.push_back(i + 24);
            }
        }
    }
    m_mesh_pos_gpu_buffer.BufferData(m_mesh_vertices);
    m_mesh_indices_gpu_buffer.BufferData(m_mesh_indices);
    m_mesh_vao.Init(
        {
            {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_mesh_pos_gpu_buffer},
            {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_mesh_pos_gpu_buffer}
        },
        m_mesh_indices_gpu_buffer);
}

void application::render_mesh() {
    m_mesh_vao.Bind();
    // m_mesh_program.Use();
    m_particle_program.Use();
    m_particle_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    m_particle_program.SetUniform("world", glm::mat4(1));

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

    glDrawElements(GL_TRIANGLES, m_mesh_indices.size(), GL_UNSIGNED_INT, 0);
}

void application::randomize_colors() {
    for (int i = 0; i < m_vertices.size(); ++i) {
        m_vertices[i].color = get_random_color();
    }
}

glm::vec3 application::hsl_to_rgb(const float h, const float s, const float l) const {
    float c = (1.0f - fabs(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    float r, g, b;
    if (h < 60.0f) {
        r = c;
        g = x;
        b = 0;
    } else if (h < 120.0f) {
        r = x;
        g = c;
        b = 0;
    } else if (h < 180.0f) {
        r = 0;
        g = c;
        b = x;
    } else if (h < 240.0f) {
        r = 0;
        g = x;
        b = c;
    } else if (h < 300.0f) {
        r = x;
        g = 0;
        b = c;
    } else {
        r = c;
        g = 0;
        b = x;
    }

    return glm::vec3(r + m, g + m, b + m);
}

glm::vec3 application::get_random_color() const {
    // Create a random number generator
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Define the range of HSL values
    std::uniform_real_distribution<float> hue_distribution(0.0f, 360.0f); // Hue range: 0° to 360°
    std::uniform_real_distribution<float> saturation_distribution(0.0f, 1.0f); // Saturation range: 0.0 to 1.0
    std::uniform_real_distribution<float> lightness_distribution(0.0f, 1.0f); // Lightness range: 0.0 to 1.0

    // Generate random HSL values
    float h = hue_distribution(gen);
    float s = saturation_distribution(gen);
    float l = lightness_distribution(gen);

    // Convert HSL to RGB
    glm::vec3 rgb = hsl_to_rgb(h, s, l);

    return rgb;
}


void application::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_axes_program.Use();
    m_axes_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    glDrawArrays(GL_LINES, 0, 6);

    draw_points(m_gpu_particle_vao, m_render_points_up_to_index);
    if (m_show_debug_sphere)
        draw_points(m_gpu_debug_sphere_vao, m_debug_sphere.size());

    if (m_show_octree)
        render_octree_boxes();

    if (m_mesh_rendering_mode == none) {
        //
    } else {
        if (m_mesh_rendering_mode == solid) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else if (m_mesh_rendering_mode == wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        init_mesh_visualization();
        render_mesh();
    }

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
