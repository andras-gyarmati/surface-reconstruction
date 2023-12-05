#include <math.h>
#include <vector>
#include <stack>
#include <random>
#include <glm/glm.hpp>
#include "application.h"
#include <glm/gtc/type_ptr.hpp>
#include "delaunay_3d.h"
#include "imgui/imgui.h"
#include "file_loader.h"
#include <Eigen/Dense>
#include <algorithm>

application::application(void) {
    m_start_eye = glm::vec3(0, 0, 0);
    m_start_at = glm::vec3(0, 1, 0);
    m_start_up = glm::vec3(0, 0, 1);
    m_virtual_camera.SetView(m_start_eye, m_start_at, m_start_up);

    strncpy_s(m_input_folder, "inputs\\garazs_kijarat", sizeof(m_input_folder));
    m_input_folder[sizeof(m_input_folder) - 1] = '\0';

    m_point_size = 6.0f;
    m_line_width = 1.0f;
    m_render_points_up_to_index = 0;
    m_mesh_vertex_cut_distance = 6.0f;

    m_show_axes = true;
    m_show_points = true;
    m_show_debug_sphere = false;
    m_show_octree = false;
    m_show_back_faces = false;
    m_show_sensor_rig_boundary = false;
    m_show_tetrahedra = false;
    m_show_non_shaded_points = false;
    m_show_texture = true;
    m_show_non_shaded_mesh = false;
    m_auto_increment_rendered_point_index = false;

    m_mesh_rendering_mode = none;
    m_octree_color = glm::vec3(0, 1.f, 0);
    m_delaunay = delaunay_3d(200.0f, glm::vec3(0.0f, 0.0f, 120.0f));
    m_sensor_rig_boundary = octree::boundary{glm::vec3(-2.3f, -1.7f, -0.5), glm::vec3(1.7f, 0.4f, 0.7f)};
}

bool application::init(SDL_Window* window) {
    m_window = window;
    m_virtual_camera.SetProj(glm::radians(60.0f), 1280.0f / 720.0f, 0.01f, 1000.0f);

    glClearColor(0.1f, 0.1f, 0.41f, 1);
    glEnable(GL_DEPTH_TEST);

    m_axes_program.Init({{GL_VERTEX_SHADER, "shaders/axes.vert"}, {GL_FRAGMENT_SHADER, "shaders/axes.frag"}});
    m_particle_program.Init({{GL_VERTEX_SHADER, "shaders/particle.vert"}, {GL_FRAGMENT_SHADER, "shaders/particle.frag"}}, {{0, "vs_in_pos"}, {1, "vs_in_col"}, {2, "vs_in_tex"}});
    m_wireframe_program.Init({{GL_VERTEX_SHADER, "shaders/wireframe.vert"}, {GL_FRAGMENT_SHADER, "shaders/wireframe.frag"}}, {{0, "vs_in_pos"}, {1, "vs_in_col"},});

    load_inputs_from_folder("inputs\\garazs_kijarat");
    init_debug_sphere();

    init_sensor_rig_boundary_visualization();

    return true;
}

void application::clean() {}

void application::reset() {}

void application::update() {
    glLineWidth(m_line_width);

    if (m_show_back_faces) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    static Uint32 last_time = SDL_GetTicks();
    const float delta_time = (float)(SDL_GetTicks() - last_time) / 1000.0f;
    m_virtual_camera.Update(delta_time);
    last_time = SDL_GetTicks();

    if (m_auto_increment_rendered_point_index && m_render_points_up_to_index < m_vertices.size()) {
        m_render_points_up_to_index += 1;
    }
}

void application::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_show_axes) {
        m_axes_program.Use();
        m_axes_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
        glDrawArrays(GL_LINES, 0, 6);
    }

    if (m_show_points)
        render_points(m_particle_vao, m_render_points_up_to_index);

    if (m_show_debug_sphere)
        render_points(m_debug_sphere_vao, m_debug_sphere.size());

    if (m_show_octree)
        render_octree_boxes();

    if (m_show_sensor_rig_boundary) {
        init_sensor_rig_boundary_visualization();
        render_sensor_rig_boundary();
    }

    if (m_mesh_rendering_mode != none) {
        if (m_mesh_rendering_mode == solid) {
            glPolygonMode(GL_FRONT, GL_FILL);
        } else if (m_mesh_rendering_mode == wireframe) {
            glPolygonMode(GL_FRONT, GL_LINE);
        }
        init_mesh_visualization();
        render_mesh();
    }

    if (m_show_tetrahedra)
        render_tetrahedra();

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

void application::load_inputs_from_folder(const std::string& folder_name) {
    const std::vector<std::string> file_paths = file_loader::get_directory_files(folder_name);
    std::string xyz_file;

    for (const auto& file : file_paths) {
        if (file.find("Dev0") != std::string::npos) {
            m_digital_camera_textures[0].FromFile(file);
            std::cout << "Loaded texture from " << file << std::endl;
        } else if (file.find("Dev1") != std::string::npos) {
            m_digital_camera_textures[1].FromFile(file);
            std::cout << "Loaded texture from " << file << std::endl;
        } else if (file.find("Dev2") != std::string::npos) {
            m_digital_camera_textures[2].FromFile(file);
            std::cout << "Loaded texture from " << file << std::endl;
        } else if (file.find(".xyz") != std::string::npos) {
            xyz_file = file;
        }
    }

    m_vertices = file_loader::load_xyz_file(xyz_file);
    m_vertex_groups.push_back(m_vertices);
    std::cout << "Loaded " << m_vertices.size() << " points from " << xyz_file << std::endl;
    m_render_points_up_to_index = m_vertices.size() - 16;
    m_digital_camera_params = file_loader::load_digital_camera_params("inputs\\CameraParametersMinimal.txt");
    std::cout << "Loaded digital camera parameters from inputs\\CameraParametersMinimal.txt" << std::endl;

    //RunRANSAC(m_vertices, 2);
    RunRANSAC(m_vertex_groups, 2);
    init_point_visualization();
    //randomize_vertex_colors(m_vertices);
    init_octree(m_vertices);
    init_octree_visualization(&m_octree);
    init_delaunay_shaded_points_segment();
    init_mesh_visualization();
}

void application::init_point_visualization() {
    //m_particle_buffer.BufferData(m_vertices);
    m_particle_buffer.BufferData(m_vertex_groups);
    m_particle_vao.Init({
        {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_particle_buffer},
        {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_particle_buffer}
    });
}

void application::init_debug_sphere() {
    m_debug_sphere.resize((m_debug_sphere_m + 1) * (m_debug_sphere_n + 1));
    for (int i = 0; i <= m_debug_sphere_n; ++i)
        for (int j = 0; j <= m_debug_sphere_m; ++j)
            m_debug_sphere[i + j * (m_debug_sphere_n + 1)] = get_sphere_pos(
                (float)(i) / (float)(m_debug_sphere_n),
                (float)(j) / (float)(m_debug_sphere_m));
    m_debug_sphere_buffer.BufferData(m_debug_sphere);
    m_debug_sphere_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_debug_sphere_buffer}});
}

void application::init_octree(const std::vector<file_loader::vertex>& vertices) {
    const auto [tlf, brb] = octree::calc_boundary(vertices);
    m_octree = octree(tlf, brb);
    for (int i = 0; i < vertices.size(); ++i) {
        if (vertices[i].position != glm::vec3(0, 0, 0)) {
            m_octree.insert(vertices[i].position);
        }
    }
}

void application::init_box(const glm::vec3& tlf, const glm::vec3& brb, std::vector<file_loader::vertex>& vertices, std::vector<int>& indices, glm::vec3 color) {
    glm::vec3 bottom_left_front(tlf.x, brb.y, tlf.z);
    glm::vec3 top_right_back(brb.x, tlf.y, brb.z);
    glm::vec3 bottom_left_back(tlf.x, brb.y, brb.z);
    glm::vec3 top_right_front(brb.x, tlf.y, tlf.z);
    glm::vec3 top_left_back(tlf.x, tlf.y, brb.z);
    glm::vec3 bottom_right_front(brb.x, brb.y, tlf.z);

    const int offset = vertices.size();

    auto pos = std::vector<file_loader::vertex>{
        // back face
        {bottom_left_back, color},
        {brb, color},
        {top_right_back, color},
        {top_left_back, color},
        // front face
        {bottom_left_front, color},
        {bottom_right_front, color},
        {top_right_front, color},
        {tlf, color},
    };
    vertices.insert(vertices.end(), pos.begin(), pos.end());

    std::vector<int> local_indices = std::vector<int>{
        // back face
        0, 1, 1, 2, 2, 3, 3, 0,
        // front face
        4, 5, 5, 6, 6, 7, 7, 4,
        // connecting edges
        0, 4, 1, 5, 2, 6, 3, 7,
    };
    for (auto& index : local_indices) {
        index += offset;
    }
    indices.insert(indices.end(), local_indices.begin(), local_indices.end());
}

void application::init_octree_visualization(const octree* root) {
    if (!root)
        return;
    m_wireframe_vertices.clear();
    m_wireframe_indices.clear();
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
            init_box(*node->m_top_left_front,
                     *node->m_bottom_right_back,
                     m_wireframe_vertices,
                     m_wireframe_indices,
                     m_octree_color);
        }
    }
    m_wireframe_vertices_buffer.BufferData(m_wireframe_vertices);
    m_wireframe_indices_buffer.BufferData(m_wireframe_indices);
    m_wireframe_vao.Init(
        {
            {
                AttributeData{
                    0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex),
                    (void*)offsetof(file_loader::vertex, position)
                },
                m_wireframe_vertices_buffer
            },
            {
                AttributeData{
                    1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)
                },
                m_wireframe_vertices_buffer
            }
        },
        m_wireframe_indices_buffer);
}

void application::init_mesh_visualization() {
    m_mesh_indices.clear();
    for (int i = 0; i < m_render_points_up_to_index; ++i) {
        if ((i % 16) != 15 && i < m_render_points_up_to_index - 16) {
            if (is_outside_of_sensor_rig_boundary(i, i + 1, i + 17) && is_mesh_vertex_cut_distance_ok(i, i + 1, i + 17)) {
                m_mesh_indices.push_back(i + 0);
                m_mesh_indices.push_back(i + 1);
                m_mesh_indices.push_back(i + 17);
            }
            if (is_outside_of_sensor_rig_boundary(i, i + 17, i + 16) && is_mesh_vertex_cut_distance_ok(i, i + 17, i + 16)) {
                m_mesh_indices.push_back(i + 0);
                m_mesh_indices.push_back(i + 17);
                m_mesh_indices.push_back(i + 16);
            }
        }
    }
    m_mesh_pos_buffer.BufferData(m_vertices);
    //m_mesh_pos_buffer.BufferData(m_vertex_groups);
    m_mesh_indices_buffer.BufferData(m_mesh_indices);
    m_mesh_vao.Init(
        {
            {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_mesh_pos_buffer},
            {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_mesh_pos_buffer}
        },
        m_mesh_indices_buffer);
}

void application::init_sensor_rig_boundary_visualization() {
    m_sensor_rig_boundary_vertices = {};
    m_sensor_rig_boundary_indices = {};

    init_box(m_sensor_rig_boundary.m_top_left_front,
             m_sensor_rig_boundary.m_bottom_right_back,
             m_sensor_rig_boundary_vertices,
             m_sensor_rig_boundary_indices,
             glm::vec3(255, 0, 0));

    m_sensor_rig_boundary_vertices_buffer.BufferData(m_sensor_rig_boundary_vertices);
    m_sensor_rig_boundary_indices_buffer.BufferData(m_sensor_rig_boundary_indices);
    m_sensor_rig_boundary_vao.Init(
        {
            {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_sensor_rig_boundary_vertices_buffer},
            {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_sensor_rig_boundary_vertices_buffer}
        },
        m_sensor_rig_boundary_indices_buffer);
}

void application::init_delaunay_shaded_points_segment() {
    m_delaunay_vertices = filter_shaded_points(m_vertices);
    init_delaunay();
}

void application::init_delaunay_cube() {
    m_delaunay_vertices = get_cube_vertices(3.0f);
    init_delaunay();
}

void application::init_delaunay() {
    m_delaunay = delaunay_3d(200.0f, glm::vec3(0.0f, 0.0f, 120.0f));
    for (int i = 0; i < std::min((int)m_delaunay_vertices.size(), 200); ++i) {
        m_delaunay.insert_point(m_delaunay_vertices[i]);
    }
    for (int i = 0; i < 4; ++i) {
        m_delaunay.cleanup_super_tetrahedron();
    }
    init_delaunay_visualization();
}

void application::init_delaunay_visualization() {
    m_tetrahedra_vertices = {};
    m_tetrahedra_indices = {};

    // const auto tetrahedra = m_delaunay.create_mesh(m_vertices);
    for (auto tetrahedron : m_delaunay.m_tetrahedra) {
        init_tetrahedron(&tetrahedron);
    }

    m_tetrahedra_vertices_buffer.BufferData(m_tetrahedra_vertices);
    m_tetrahedra_indices_buffer.BufferData(m_tetrahedra_indices);
    m_tetrahedra_vao.Init(
        {
            {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_tetrahedra_vertices_buffer},
            {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_tetrahedra_vertices_buffer}
        },
        m_tetrahedra_indices_buffer);
}

void application::init_tetrahedron(const delaunay_3d::tetrahedron* tetrahedron) {
    const glm::vec3 random_color = get_random_color();
    const int offset = m_tetrahedra_vertices.size();
    for (const glm::vec3 vert : tetrahedron->m_vertices) {
        m_tetrahedra_vertices.push_back({vert /*+ (get_random_color() * 0.4f)*/, random_color});
    }

    std::vector<int> indices = std::vector<int>{
        0, 1, 2,
        1, 0, 3,
        2, 1, 3,
        0, 2, 3
    };
    for (int& index : indices) {
        index += offset;
    }
    m_tetrahedra_indices.insert(m_tetrahedra_indices.end(), indices.begin(), indices.end());
}

void application::render_imgui() {
    glm::vec3 eye = m_virtual_camera.GetEye();
    glm::vec3 at = m_virtual_camera.GetAt();
    glm::vec3 up = m_virtual_camera.GetUp();
    float cam_speed = m_virtual_camera.GetSpeed();
    if (ImGui::Begin("menu")) {
        if (ImGui::Button("toggle fullscreen")) {
            toggle_fullscreen(m_window);
        }
        ImGui::SameLine();
        ImGui::Checkbox("show axes", &m_show_axes);
        ImGui::SliderFloat("line width", &m_line_width, 1.0f, 10.0f);
        if (ImGui::CollapsingHeader("loading")) {
            if (ImGui::Button("load garazs_kijarat")) {
                load_inputs_from_folder("inputs\\garazs_kijarat");
            }
            ImGui::SameLine();
            if (ImGui::Button("load elte_logo")) {
                load_inputs_from_folder("inputs\\elte_logo");
            }
            ImGui::SameLine();
            if (ImGui::Button("load parkolo_gomb")) {
                load_inputs_from_folder("inputs\\parkolo_gomb");
            }
            ImGui::PushID("m_input_folder");
            ImGui::InputText("", m_input_folder, sizeof(m_input_folder));
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button("load folder")) {
                load_inputs_from_folder(m_input_folder);
            }
        }
        if (ImGui::CollapsingHeader("points")) {
            ImGui::Checkbox("show points", &m_show_points);
            ImGui::SameLine();
            ImGui::Checkbox("show texture", &m_show_texture);
            ImGui::SameLine();
            ImGui::Checkbox("show non shaded points", &m_show_non_shaded_points);
            ImGui::Checkbox("show debug sphere", &m_show_debug_sphere);
            ImGui::SliderFloat("point size", &m_point_size, 1.0f, 30.0f);
            ImGui::Checkbox("auto increment rendered point index", &m_auto_increment_rendered_point_index);
            if (ImGui::Button("-1")) {
                if (m_render_points_up_to_index > 0) {
                    --m_render_points_up_to_index;
                }
            }
            ImGui::SameLine();
            ImGui::PushID("m_render_points_up_to_index");
            ImGui::SliderInt("", &m_render_points_up_to_index, 0, m_vertices.size());
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button("+1")) {
                if (m_render_points_up_to_index + 1 <= m_vertices.size()) {
                    ++m_render_points_up_to_index;
                }
            }
        }
        if (ImGui::CollapsingHeader("mesh")) {
            ImGui::Checkbox("show non shaded mesh", &m_show_non_shaded_mesh);
            ImGui::SameLine();
            ImGui::Checkbox("show back faces", &m_show_back_faces);
            ImGui::Text("mesh rendering mode");
            if (ImGui::Button("none")) {
                m_mesh_rendering_mode = none;
            }
            ImGui::SameLine();
            if (ImGui::Button("wireframe")) {
                m_mesh_rendering_mode = wireframe;
            }
            ImGui::SameLine();
            if (ImGui::Button("solid")) {
                m_mesh_rendering_mode = solid;
            }
            ImGui::SliderFloat("mesh vertex cut distance", &m_mesh_vertex_cut_distance, 0.1f, 50.0f);
        }
        if (ImGui::CollapsingHeader("sensor rig")) {
            ImGui::Checkbox("show sensor rig boundary", &m_show_sensor_rig_boundary);
            ImGui::SliderFloat3("sensor rig top left front", &m_sensor_rig_boundary.m_top_left_front[0], -4.0f, -0.1f);
            ImGui::SliderFloat3("sensor rig bottom right back", &m_sensor_rig_boundary.m_bottom_right_back[0], 0.1f, 4.0f);
        }
        if (ImGui::CollapsingHeader("octree")) {
            ImGui::Checkbox("show octree", &m_show_octree);
            ImGui::ColorEdit3("octree color", &m_octree_color[0]);
            if (ImGui::Button("apply octree color")) {
                init_octree_visualization(&m_octree);
            }
        }
        if (ImGui::CollapsingHeader("delaunay")) {
            if (ImGui::Button("init delaunay cube")) {
                init_delaunay_cube();
            }
            if (ImGui::Button("init delaunay shaded points segment")) {
                init_delaunay_shaded_points_segment();
            }
            ImGui::Checkbox("show tetrahedra", &m_show_tetrahedra);
        }
        if (ImGui::CollapsingHeader("camera")) {
            ImGui::SliderFloat("cam speed", &cam_speed, 0.1f, 40.0f);
            if (ImGui::Button("reset camera")) {
                eye = m_start_eye;
                at = m_start_at;
                up = m_start_up;
            }
            ImGui::SameLine();
            if (ImGui::Button("set camera far")) {
                eye = glm::vec3(100, 100, 100);
                at = glm::vec3(0, 0, 0);
                up = glm::vec3(0, 0, 1);
            }
        }
    }
    ImGui::End();
    m_virtual_camera.SetView(eye, at, up);
    m_virtual_camera.SetSpeed(cam_speed);
}

void application::render_points(VertexArrayObject& vao, const size_t size) {
    vao.Bind();
    set_particle_program_uniforms(m_show_non_shaded_points);
    m_particle_program.SetUniform("show_texture", (int)m_show_texture);
    glEnable(GL_PROGRAM_POINT_SIZE);
    m_particle_program.SetUniform("point_size", m_point_size);
    glDrawArrays(GL_POINTS, 0, size);
    glDisable(GL_PROGRAM_POINT_SIZE);
    vao.Unbind();
}

void application::render_octree_boxes() {
    m_wireframe_vao.Bind();
    glPolygonMode(GL_FRONT, GL_LINE);
    m_wireframe_program.Use();
    m_wireframe_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    glDrawElements(GL_LINES, m_wireframe_indices.size(), GL_UNSIGNED_INT, nullptr);
    m_wireframe_vao.Unbind();
}

void application::render_mesh() {
    m_mesh_vao.Bind();
    set_particle_program_uniforms(m_show_non_shaded_mesh);
    glDrawElements(GL_TRIANGLES, m_mesh_indices.size(), GL_UNSIGNED_INT, 0);
    m_mesh_vao.Unbind();
}

void application::render_sensor_rig_boundary() {
    m_sensor_rig_boundary_vao.Bind();
    glPolygonMode(GL_FRONT, GL_LINE);
    m_wireframe_program.Use();
    m_wireframe_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    glDrawElements(GL_LINES, m_sensor_rig_boundary_indices.size(), GL_UNSIGNED_INT, nullptr);
    m_sensor_rig_boundary_vao.Unbind();
}

void application::render_tetrahedra() {
    glDisable(GL_CULL_FACE);
    m_tetrahedra_vao.Bind();
    glPolygonMode(GL_FRONT, GL_LINE);
    m_wireframe_program.Use();
    m_wireframe_program.SetUniform("mvp", m_virtual_camera.GetViewProj());
    glDrawElements(GL_TRIANGLES, m_tetrahedra_indices.size(), GL_UNSIGNED_INT, nullptr);
    m_tetrahedra_vao.Unbind();
    if (m_show_back_faces) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
}

std::vector<file_loader::vertex> application::get_cube_vertices(const float side_len) {
    return {
        {glm::vec3(+side_len, +side_len, +side_len), glm::vec3(1)},
        {glm::vec3(+side_len, +side_len, -side_len), glm::vec3(1)},
        {glm::vec3(+side_len, -side_len, +side_len), glm::vec3(1)},
        {glm::vec3(+side_len, -side_len, -side_len), glm::vec3(1)},
        {glm::vec3(-side_len, +side_len, +side_len), glm::vec3(1)},
        {glm::vec3(-side_len, +side_len, -side_len), glm::vec3(1)},
        {glm::vec3(-side_len, -side_len, +side_len), glm::vec3(1)},
        {glm::vec3(-side_len, -side_len, -side_len), glm::vec3(1)},
    };
}

std::vector<file_loader::vertex> application::filter_shaded_points(const std::vector<file_loader::vertex>& points) {
    std::vector<file_loader::vertex> shaded_points;
    glm::mat3 cam_k = m_digital_camera_params.get_cam_k();
    glm::mat3 cam_r[3];
    glm::vec3 cam_t[3];
    cam_r[0] = m_digital_camera_params.devices[0].r;
    cam_r[1] = m_digital_camera_params.devices[1].r;
    cam_r[2] = m_digital_camera_params.devices[2].r;
    cam_t[0] = m_digital_camera_params.devices[0].t;
    cam_t[1] = m_digital_camera_params.devices[1].t;
    cam_t[2] = m_digital_camera_params.devices[2].t;

    for (const auto& point : points) {
        bool is_shaded = false;
        for (int i = 0; i < 3; ++i) {
            glm::vec3 p_tmp = cam_r[i] * (point.position - cam_t[i]);
            const float dist = p_tmp.z;
            p_tmp /= p_tmp.z;
            glm::vec2 p_c;
            p_c.x = cam_k[0][0] * p_tmp.x + cam_k[0][2];
            p_c.y = cam_k[1][1] * -p_tmp.y + cam_k[1][2];
            if (dist > 0 && p_c.x >= 0 && p_c.x <= 960 && p_c.y >= 0 && p_c.y <= 600) {
                is_shaded = true;
            }
        }
        if (is_shaded && !m_sensor_rig_boundary.contains(point.position)) {
            shaded_points.push_back(point);
        }
    }
    return shaded_points;
}

bool application::is_mesh_vertex_cut_distance_ok(const int i0, const int i1, const int i2) const {
    return glm::distance(m_vertices[i0].position, m_vertices[i1].position) < m_mesh_vertex_cut_distance &&
        glm::distance(m_vertices[i1].position, m_vertices[i2].position) < m_mesh_vertex_cut_distance &&
        glm::distance(m_vertices[i2].position, m_vertices[i0].position) < m_mesh_vertex_cut_distance;
}

bool application::is_outside_of_sensor_rig_boundary(const int i0, const int i1, const int i2) const {
    return !(m_sensor_rig_boundary.contains(m_vertices[i0].position) ||
        m_sensor_rig_boundary.contains(m_vertices[i1].position) ||
        m_sensor_rig_boundary.contains(m_vertices[i2].position));
}

void application::set_particle_program_uniforms(bool show_non_shaded) {
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
    m_particle_program.SetUniform("show_non_shaded", (int)show_non_shaded);
}

void application::randomize_vertex_colors(std::vector<file_loader::vertex>& vertices) const {
    for (file_loader::vertex& vertex : vertices) {
        vertex.color = get_random_color();
    }
}

glm::vec3 application::hsl_to_rgb(const float h, const float s, const float l) const {
    const float c = (1.0f - fabs(2.0f * l - 1.0f)) * s;
    const float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    const float m = l - c / 2.0f;

    glm::vec3 color;
    if (h < 60.0f) {
        color = glm::vec3(c, x, 0);
    } else if (h < 120.0f) {
        color = glm::vec3(x, c, 0);
    } else if (h < 180.0f) {
        color = glm::vec3(0, c, x);
    } else if (h < 240.0f) {
        color = glm::vec3(0, x, c);
    } else if (h < 300.0f) {
        color = glm::vec3(x, 0, c);
    } else {
        color = glm::vec3(c, 0, x);
    }

    return color + m;
}

glm::vec3 application::get_random_color() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> hue_distribution(0.0f, 360.0f);
    const float h = hue_distribution(gen);
    const float s = 0.5f;
    const float l = 0.5f;
    return hsl_to_rgb(h, s, l);
}

glm::vec3 application::get_sphere_pos(const float u, const float v) const {
    const float th = u * 2.0f * (float)(M_PI);
    const float fi = v * 2.0f * (float)(M_PI);
    constexpr float r = 2.0f;

    return {
        r * sin(th) * cos(fi),
        r * sin(th) * sin(fi),
        r * cos(th)
    };
}

void application::toggle_fullscreen(SDL_Window* win) {
    const Uint32 window_flags = SDL_GetWindowFlags(win);
    if (window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(win, 0);
        SDL_SetWindowResizable(win, SDL_TRUE);
        SDL_SetWindowPosition(win, 10, 40);
        SDL_SetWindowSize(win, 1280, 720);
    } else {
        SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_SetWindowResizable(win, SDL_FALSE);
    }
}

float* application::EstimatePlaneImplicit(const std::vector<file_loader::vertex*>& pts) {
    const int num = pts.size();

    Eigen::MatrixXf Cfs(num, 4);

    for (int i = 0; i < num; i++) {
        file_loader::vertex* pt = pts.at(i);
        Cfs(i, 0) = pt->position.x;
        Cfs(i, 1) = pt->position.y;
        Cfs(i, 2) = pt->position.z;
        Cfs(i, 3) = 1.0f;
    }

    Eigen::MatrixXf mtx = Cfs.transpose() * Cfs;
    Eigen::EigenSolver<Eigen::MatrixXf> es(mtx);

    const int lowestEigenValueIndex = std::min({0,1,2,3},
        [&es](int v1, int v2) {
            return es.eigenvalues()[v1].real() < es.eigenvalues()[v2].real();
        });

    float A = es.eigenvectors().col(lowestEigenValueIndex)(0).real();
    float B = es.eigenvectors().col(lowestEigenValueIndex)(1).real();
    float C = es.eigenvectors().col(lowestEigenValueIndex)(2).real();
    float D = es.eigenvectors().col(lowestEigenValueIndex)(3).real();

    float norm = std::sqrt(A * A + B * B + C * C);

    float* ret = new float[4];
    ret[0] = A / norm;
    ret[1] = B / norm;
    ret[2] = C / norm;
    ret[3] = D / norm;

    return ret;
}

application::RANSACDiffs application::PlanePointRANSACDifferences(const std::vector<file_loader::vertex*>& pts, float* plane, float threshold) {
    size_t num = pts.size();

    float A = plane[0];
    float B = plane[1];
    float C = plane[2];
    float D = plane[3];

    RANSACDiffs ret;

    std::vector<bool> isInliers;
    std::vector<float> distances;

    int inlierCounter = 0;
    for (int idx = 0; idx < num; idx++) {
        file_loader::vertex* pt = pts.at(idx);
        float diff = fabs(A * pt->position.x + B * pt->position.y + C * pt->position.z + D);
        distances.push_back(diff);
        if (diff < threshold) {
            isInliers.push_back(true);
            ++inlierCounter;
        }
        else {
            isInliers.push_back(false);
        }
    }

    ret.distances = distances;
    ret.isInliers = isInliers;
    ret.inliersNum = inlierCounter;

    return ret;
}

float* application::EstimatePlaneRANSAC(std::vector<file_loader::vertex*>& pts, float threshold, int iterNum) {
    size_t num = pts.size();

    int bestSampleInlierNum = 0;
    float bestPlane[4];

    for (int iter = 0; iter < iterNum; iter++) {
        int index1 = rand() % num;
        int index2 = rand() % num;

        while (index2 == index1) {
            index2 = rand() % num;
        }
        int index3 = rand() % num;
        while (index3 == index1 || index3 == index2) {
            index3 = rand() % num;
        }

        file_loader::vertex* pt1 = pts.at(index1);
        file_loader::vertex* pt2 = pts.at(index2);
        file_loader::vertex* pt3 = pts.at(index3);

        std::vector<file_loader::vertex*> minimalSample;
        minimalSample.push_back(pt1);
        minimalSample.push_back(pt2);
        minimalSample.push_back(pt3);

        float* samplePlane = EstimatePlaneImplicit(minimalSample);

        RANSACDiffs sampleResult = PlanePointRANSACDifferences(pts, samplePlane, threshold);

        if (sampleResult.inliersNum > bestSampleInlierNum) {
            bestSampleInlierNum = sampleResult.inliersNum;
            for (int i = 0; i < 4; ++i) {
                bestPlane[i] = samplePlane[i];
            }
        }

        delete[] samplePlane;
    }

    RANSACDiffs bestResult = PlanePointRANSACDifferences(pts, bestPlane, threshold);
    std::cout << "Best plane params: " << bestPlane[0] << " " << bestPlane[1] << " " << bestPlane[2] << "\n";

    std::vector<file_loader::vertex*> inlierPts;

    for (int idx = 0; idx < num; idx++) {
        if (bestResult.isInliers.at(idx)) {
            inlierPts.push_back(pts.at(idx));
        }
    }

    float* finalPlane = EstimatePlaneImplicit(inlierPts);
    return finalPlane;
}

bool operator==(const file_loader::vertex& l, const file_loader::vertex& r) {
    return l.position.x == r.position.x && l.position.y == r.position.y && l.position.z == r.position.z;
}

void application::RunRANSAC(std::vector<std::vector<file_loader::vertex>>& points,const int iterations) {
    // Constants, replace them as needed
    const float FILTER_LOWEST_DISTANCE = 1.5f;
    const float THERSHOLD = 0.15f;
    const int RANSAC_ITER = 1000;

    std::vector<file_loader::vertex*> filteredPoints;
    float hValue = 0.0f;

    for (int i = 0; i < iterations; i++) {
        filteredPoints.clear();
        glm::vec3 iterColor = hsl_to_rgb(hValue, 0.5f, 0.5f);

        for (auto& point : points[0]) {
            float distFromOrigo = glm::length(glm::vec3(point.position.x, point.position.y, point.position.z));

            if (distFromOrigo > FILTER_LOWEST_DISTANCE) {
                filteredPoints.push_back(&point);
            }
        }

        size_t num = filteredPoints.size();
        std::cout << "Point num: " << num << "\n";
    
        //parameters of best fitting plane determined by RANSAC
        float* planeParams = EstimatePlaneRANSAC(filteredPoints, THERSHOLD, RANSAC_ITER);

        std::cout << "Plane params RANSAC:" << std::endl;
        std::cout << "A:" << planeParams[0] << " B:" << planeParams[1]
            << " C:" << planeParams[2] << " D:" << planeParams[3] << std::endl;

        //Color inlier points
        RANSACDiffs differences = PlanePointRANSACDifferences(filteredPoints, planeParams, THERSHOLD);
        std::vector<file_loader::vertex> points_to_group;
        for (int idx = 0; idx < num; idx++) {
            if (differences.isInliers.at(idx)) {
                filteredPoints.at(idx)->color = iterColor;
                points_to_group.push_back(*filteredPoints.at(idx));
                points[0].erase(std::remove(points[0].begin(), points[0].end(), *(filteredPoints.at(idx))), points[0].end());
            }
        }
        hValue += 360.0 / iterations;
        points.push_back(points_to_group);

        delete[] planeParams;
    }
}