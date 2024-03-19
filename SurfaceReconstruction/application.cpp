#include <vector>
#include <stack>
#include <random>
#include <glm/glm.hpp>
#include "application.h"
#include "imgui/imgui.h"
#include "file_loader.h"
#include <Eigen/Dense>

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
    m_uv_stretch_scalar = 0.003f;
    m_normal_cut_scalar = 0.003f;

    m_bfs_paint_animation_speed = 1.0f;
    m_time_since_last_bfs_paint = 0.0f;
    m_bfs_epsilon = 0.93f;

    m_show_axes = false;
    m_show_points = false;
    m_show_debug_sphere = false;
    m_show_octree = false;
    m_show_back_faces = false;
    m_show_sensor_rig_boundary = false;
    m_show_normal = false;
    m_show_uv_stretch = false;
    m_show_bfs_col = false;
    m_show_color = true;
    m_show_ransac = false;
    m_show_non_shaded_points = false;
    m_show_non_shaded_mesh = false;
    m_auto_increment_rendered_point_index = false;

    m_mesh_rendering_mode = none;
    m_octree_color = glm::vec3(0, 1.f, 0);
    m_sensor_rig_boundary = octree::boundary{glm::vec3(-2.3f, -1.7f, -0.5), glm::vec3(1.7f, 0.4f, 0.7f)};
}

bool application::init(SDL_Window* window) {
    m_window = window;
    m_virtual_camera.SetProj(glm::radians(60.0f), 1280.0f / 720.0f, 0.01f, 1000.0f);

    glClearColor(0.1f, 0.1f, 0.41f, 1);
    glEnable(GL_DEPTH_TEST);

    m_axes_program.Init({{GL_VERTEX_SHADER, "shaders/axes.vert"}, {GL_FRAGMENT_SHADER, "shaders/axes.frag"}});
    m_particle_program.Init({{GL_VERTEX_SHADER, "shaders/particle.vert"}, {GL_FRAGMENT_SHADER, "shaders/particle.frag"}},
                            {
                                {0, "vs_in_pos"},
                                {1, "vs_in_col"},
                                {2, "vs_in_ransac"},
                                {3, "vs_in_norm"},
                                {4, "vs_in_uv_stretch"},
                                {5, "vs_in_bfs_col"}
                            });
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
    m_time_since_last_bfs_paint += delta_time;
    // std::cout << "m_time_since_last_bfs_paint: " << m_time_since_last_bfs_paint << std::endl;

    const std::vector neighbors = {-1, +1, -16, +16, -17, -15, +15, +17};
    if ((m_time_since_last_bfs_paint > (1 - m_bfs_paint_animation_speed)) && !m_vertices_queue.empty()) {
        m_time_since_last_bfs_paint = 0.0f;
        const int i = m_vertices_queue.front();
        m_vertices_queue.pop();
        std::cout << i << "\n";
        // processed vertexes are blue
        m_vertices[i].bfs_col = glm::vec3(0, 0, 1);
        if ((i % 16) != 15 && (i % 16) != 0 && 15 < i && i < m_render_points_up_to_index - 16) {
            for (const int neighbor : neighbors) {
                const float dot = fabs(glm::dot(m_vertices[i + neighbor].normal, m_vertices[i].normal));
                if (m_vertices[i + neighbor].bfs_col == glm::vec3(1) && dot > m_bfs_epsilon && !m_vertices[i + neighbor].is_grouped) {
                    m_vertices_queue.push(i + neighbor);
                    // color the neighbor in the queue to red
                    m_vertices[i + neighbor].bfs_col = glm::vec3(1, 0, 0);
                }
            }
        }
    } else {
        //std::cout << "m_vertices_queue is empty\n";
    }

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

    if (m_show_points) {
        init_point_visualization();
        render_points(m_particle_vao, m_vertices.get_points_to_render().size());
    }

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

    m_vertices = VertexSet(file_loader::load_xyz_file(xyz_file));
    m_vertex_groups.clear();
    m_cuts = std::vector<cut>(m_vertices.size());
    std::cout << "Loaded " << m_vertices.size() << " points from " << xyz_file << std::endl;
    m_render_points_up_to_index = m_vertices.size() - 16;
    m_digital_camera_params = file_loader::load_digital_camera_params("inputs\\CameraParametersMinimal.txt");
    std::cout << "Loaded digital camera parameters from inputs\\CameraParametersMinimal.txt" << std::endl;
    
    RunRANSAC(m_ransac_object_count);
    //randomize_vertex_colors(m_vertices);
    set_uvs(m_vertices.get_points());
    // init_octree(m_vertices);
    // init_octree_visualization(&m_octree);
    // init_delaunay_shaded_points_segment();
    init_point_visualization();
    init_mesh_visualization();

    // set all vertices bfs color to white for bfs painting algo
    for (auto& vertex : m_vertices.get_points()) {
        vertex.bfs_col = glm::vec3(1);
    }

    // select a random vertex and put in in m_vertices_queue from shaded points, use filter_shaded_points function
    std::random_device rd;
    std::mt19937 gen(rd());
    //const auto shaded_points = filter_shaded_points(m_vertices.get_non_grouped());
    std::uniform_int_distribution<> dis(0, m_vertices.get_non_grouped().size() - 1);
    //int random_index;
    int random_index = m_vertices.get_non_grouped()[dis(gen)];
    m_vertices_queue.push(random_index);
    //m_vertices_queue.push(1809);
    //m_vertices_queue.push(2309);
}

void application::init_point_visualization() {
    m_particle_buffer.BufferData(m_vertices.get_points_to_render());
    m_particle_vao.Init({
        {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_particle_buffer},
        {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_particle_buffer},
        {AttributeData{2, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, ransac)}, m_particle_buffer},
        {AttributeData{3, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, normal)}, m_particle_buffer},
        {AttributeData{4, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, uv_stretch)}, m_particle_buffer},
        {AttributeData{5, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, bfs_col)}, m_particle_buffer}
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
            {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_wireframe_vertices_buffer},
            {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_wireframe_vertices_buffer},
            {AttributeData{2, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, normal)}, m_wireframe_vertices_buffer}
        },
        m_wireframe_indices_buffer);
}

void application::init_mesh_visualization() {
    std::vector<file_loader::vertex> points = m_vertices.get_points_to_render();

    for (int i = 0; i < points.size(); i++) {
        const float v_dist_from_center = glm::distance(points[i].position, glm::vec3(0, 0, 0));
        if (v_dist_from_center > m_max_dist_from_center) {
            m_max_dist_from_center = v_dist_from_center;
        }
    }

    for (int i = 0; i < points.size(); i++) {
        const float v_dist_uv_dist_ratio_norm = m_cuts[i].ratio / m_max_v_dist_uv_dist_ratio;
        const float v_dist_from_center_norm = glm::distance(points[i].position, glm::vec3(0, 0, 0)) / m_max_dist_from_center;
        const float uv_stretch = m_cuts[i].dist / v_dist_from_center_norm * m_uv_stretch_scalar;
        m_cuts[i].uv_stretch = uv_stretch;
        points[i].uv_stretch = hsl_to_rgb(uv_stretch * 360.0f / 2.0f, 0.5f, 0.5f);
    }

    m_mesh_indices.clear();
    for (int i = 0; i < m_render_points_up_to_index; ++i) {
        if ((i % 16) != 15 && i < m_render_points_up_to_index - 16) {
            if (!is_triangle_should_be_excluded(i, i + 1, i + 17) && is_outside_of_sensor_rig_boundary(i, i + 1, i + 17) && is_mesh_vertex_cut_distance_ok(i, i + 1, i + 17)) {
                m_mesh_indices.push_back(i + 0);
                m_mesh_indices.push_back(i + 1);
                m_mesh_indices.push_back(i + 17);
            }
            if (!is_triangle_should_be_excluded(i, i + 17, i + 16) && is_outside_of_sensor_rig_boundary(i, i + 17, i + 16) && is_mesh_vertex_cut_distance_ok(i, i + 17, i + 16)) {
                m_mesh_indices.push_back(i + 0);
                m_mesh_indices.push_back(i + 17);
                m_mesh_indices.push_back(i + 16);
            }
        }
    }

    // for (int i = 0; i < m_vertices.size(); i++) {
    //     float v_dist_norm = m_cuts[i].dist / m_max_dist;
    //     float v_dist_from_center_norm = glm::distance(m_vertices[i].position, glm::vec3(0, 0, 0)) / m_max_dist_from_center;
    //     float v_dist_corrected = v_dist_norm * v_dist_from_center_norm * m_mesh_vertex_cut_distance;
    //     // log values to console
    //     // std::cout << "v_dist_norm: " << v_dist_norm << std::endl;
    //     // std::cout << "v_dist_from_center_norm: " << v_dist_from_center_norm << std::endl;
    //     // std::cout << "v_dist_corrected: " << v_dist_corrected << std::endl << std::endl;
    //     m_vertices[i].color = hsl_to_rgb(v_dist_corrected * 360.0f / 2.0f, 0.5f, 0.5f);
    // }

    m_mesh_pos_buffer.BufferData(m_vertices.get_points_to_render());
    m_mesh_indices_buffer.BufferData(m_mesh_indices);
    m_mesh_vao.Init(
        {
            {AttributeData{0, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, position)}, m_mesh_pos_buffer},
            {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_mesh_pos_buffer},
            {AttributeData{2, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, ransac)}, m_mesh_pos_buffer},
            {AttributeData{3, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, normal)}, m_mesh_pos_buffer},
            {AttributeData{4, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, uv_stretch)}, m_mesh_pos_buffer},
            {AttributeData{5, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, bfs_col)}, m_mesh_pos_buffer}
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
            {AttributeData{1, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, color)}, m_sensor_rig_boundary_vertices_buffer},
            {AttributeData{2, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, ransac)}, m_sensor_rig_boundary_vertices_buffer},
            {AttributeData{3, 3, GL_FLOAT, GL_FALSE, sizeof(file_loader::vertex), (void*)offsetof(file_loader::vertex, normal)}, m_sensor_rig_boundary_vertices_buffer}
        },
        m_sensor_rig_boundary_indices_buffer);
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
            ImGui::SameLine();
            ImGui::Checkbox("show non shaded points", &m_show_non_shaded_points);
            ImGui::Checkbox("show normal", &m_show_normal);
            ImGui::Checkbox("show ransac", &m_show_ransac);
            ImGui::Checkbox("show uv stretch", &m_show_uv_stretch);
            ImGui::Checkbox("show bfs color", &m_show_bfs_col);
            ImGui::Checkbox("show color", &m_show_color);
            ImGui::SameLine();
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
        if (ImGui::CollapsingHeader("ransac")) {
            ImGui::Text("ransac planes: %d", m_ransac_object_count);
            ImGui::SameLine();
            if (ImGui::Button("-1")) {
                if(m_ransac_object_count > 0)
                    m_ransac_object_count--;
            }
            ImGui::SameLine();
            if (ImGui::Button("+1")) {
                m_ransac_object_count++;
            }
            ImGui::SliderFloat("ransac threshold", &m_ransac_threshold, 0.01f, 2.0f);
            ImGui::SliderInt("ransac iterations", &m_ransac_iter, 1, 5000);

            //if (ImGui::Button("rerun ransac")) {
            //    RunRANSAC(m_vertices.get_points(), m_vertex_groups, m_ransac_object_count);
            //}

            ImGui::Text("plane visibility");
            for (int i = 0; i < m_vertices.group_count(); i++) {
                char text[100];
                snprintf(text, 64, "group %d", i + 1);
                ImGui::Checkbox(text, (bool*)(&m_vertices.get_show_groups()[i]));
            }
            //ImGui::Checkbox("non grouped", (bool*)(&m_show_vertex_groups[m_vertex_groups.size() - 1]));
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
            ImGui::SliderFloat("uv stretch scalar", &m_uv_stretch_scalar, 0.00001f, 0.01f);
            ImGui::SliderFloat("normal cut scalar", &m_normal_cut_scalar, 0.00001f, 1.f);
            ImGui::SliderFloat("bfs paint animation speed", &m_bfs_paint_animation_speed, 0.001f, 1.f);
        }
        if (ImGui::CollapsingHeader("sensor rig")) {
            ImGui::Checkbox("show sensor rig boundary", &m_show_sensor_rig_boundary);
            ImGui::SliderFloat3("sensor rig top left front", &m_sensor_rig_boundary.m_top_left_front[0], -4.0f, -0.1f);
            ImGui::SliderFloat3("sensor rig bottom right back", &m_sensor_rig_boundary.m_bottom_right_back[0], 0.1f, 4.0f);
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
    glDrawElements(GL_TRIANGLES, m_mesh_indices.size(), GL_UNSIGNED_INT, nullptr);
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
    return fabs(dot(m_vertices[i0].normal, m_vertices[i0].position)) > m_normal_cut_scalar &&
        fabs(dot(m_vertices[i1].normal, m_vertices[i1].position)) > m_normal_cut_scalar &&
        fabs(dot(m_vertices[i2].normal, m_vertices[i2].position)) > m_normal_cut_scalar;
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
    m_particle_program.SetUniform("show_normal", (int)m_show_normal);
    m_particle_program.SetUniform("show_uv_stretch", (int)m_show_uv_stretch);
    m_particle_program.SetUniform("show_bfs_col", (int)m_show_bfs_col);
    m_particle_program.SetUniform("show_color", (int)m_show_color);
    m_particle_program.SetUniform("show_ransac", (int)m_show_ransac);
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

float* application::EstimatePlaneImplicit(const std::vector<int>& pts) {
    const size_t num = pts.size();

    Eigen::MatrixXf Cfs(num, 4);

    for (size_t i = 0; i < num; i++) {
        file_loader::vertex pt = m_vertices[pts[i]];
        Cfs(i, 0) = pt.position.x;
        Cfs(i, 1) = pt.position.y;
        Cfs(i, 2) = pt.position.z;
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

application::RANSACDiffs application::PlanePointRANSACDifferences(const std::vector<int>& pts, float* plane, float threshold) {
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
        file_loader::vertex pt = m_vertices[pts[idx]];
        float diff = fabs(A * pt.position.x + B * pt.position.y + C * pt.position.z + D);
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

float* application::EstimatePlaneRANSAC(const std::vector<int>& pts, float threshold, int iterNum) {
    size_t num = pts.size();

    int bestSampleInlierNum = 0;
    float bestPlane[4];

    for (Uint32 iter = 0; iter < iterNum; iter++) {
        int index1 = rand() % num;
        int index2 = rand() % num;

        while (index2 == index1) {
            index2 = rand() % num;
        }
        int index3 = rand() % num;
        while (index3 == index1 || index3 == index2) {
            index3 = rand() % num;
        }

        const std::vector<int> minimalSample = {index1, index2, index3};

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

    std::vector<int> inlierPts;

    for (int idx = 0; idx < num; idx++) {
        if (bestResult.isInliers.at(idx)) {
            inlierPts.push_back(pts[idx]);
        }
    }

    float* finalPlane = EstimatePlaneImplicit(inlierPts);
    return finalPlane;
}

void application::RunRANSAC(const int iterations) {
    //for (auto& point : points) {
    //    point.ransac = glm::vec3(0, 0, 0);
    //}

    for (size_t i = 0; i < m_vertices.size(); i++)
    {
        m_vertices[i].ransac = glm::vec3(0);
    }

    // Constants, replace them as needed
    const float FILTER_LOWEST_DISTANCE = 1.5f;
    const float THERSHOLD = m_ransac_threshold;
    const int RANSAC_ITER = m_ransac_iter;

    std::vector<int> filteredPoints;
    float hValue = 0.0f;

    for (int i = 0; i < iterations; i++) {
        filteredPoints.clear();
        glm::vec3 iterColor = hsl_to_rgb(hValue, 0.5f, 0.5f);

        for (size_t j = 0; j < m_vertices.size(); j++) 
        {
            float distFromOrigo = glm::length(glm::vec3(m_vertices[j].position.x, m_vertices[j].position.y, m_vertices[j].position.z));

            if (distFromOrigo > FILTER_LOWEST_DISTANCE && !m_vertices[j].is_grouped) {
                filteredPoints.push_back(j);
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
        std::cout << differences.inliersNum << std::endl;
        std::vector<int> points_to_group;
        for (int idx = 0; idx < num; idx++) {
            if (differences.isInliers.at(idx)) {
                m_vertices[filteredPoints[idx]].ransac = iterColor;
                points_to_group.push_back(filteredPoints[idx]);
            }
        }
        m_vertices.create_group(points_to_group);
        hValue += 360.0f / iterations;
        std::cout << points_to_group.size() << "\n";

        delete[] planeParams;
    }
}