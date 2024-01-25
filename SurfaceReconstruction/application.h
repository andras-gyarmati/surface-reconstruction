#pragma once
#include <SDL.h>
#include <glm/glm.hpp>
#include "Includes/ProgramObject.h"
#include "Includes/BufferObject.h"
#include "Includes/VertexArrayObject.h"
#include "Includes/TextureObject.h"
#include "Includes/gCamera.h"
#include <vector>
#include "delaunay_3d.h"
#include "file_loader.h"
#include "octree.h"
#include <cstdlib>

enum mesh_rendering_mode {
    none = 0,
    wireframe = 1,
    solid = 2
};

class application {
public:
    // constructor destructor
    application(void);
    ~application(void) = default;

    // application lifecycle methods
    bool init(SDL_Window* window);
    void clean();
    void reset();
    void update();
    void render();

    // user io event handling methods
    void keyboard_down(const SDL_KeyboardEvent&);
    void keyboard_up(const SDL_KeyboardEvent&);
    void mouse_move(const SDL_MouseMotionEvent&);
    void mouse_down(const SDL_MouseButtonEvent&);
    void mouse_up(const SDL_MouseButtonEvent&);
    void mouse_wheel(const SDL_MouseWheelEvent&);
    void resize(int, int);

    // file input
    void load_inputs_from_folder(const std::string& folder_name);

    // init methods
    void init_point_visualization();
    void init_debug_sphere();
    void init_octree(const std::vector<file_loader::vertex>& vertices);
    static void init_box(const glm::vec3& tlf, const glm::vec3& brb, std::vector<file_loader::vertex>& vertices, std::vector<int>& indices, glm::vec3 color);
    void init_octree_visualization(const octree* root);
    void init_mesh_visualization();
    void init_sensor_rig_boundary_visualization();
    void init_delaunay_shaded_points_segment();
    void init_delaunay_cube();
    void init_delaunay();
    void init_delaunay_visualization();
    void init_tetrahedron(const delaunay_3d::tetrahedron* tetrahedron);

    // render methods
    void render_imgui();
    void render_points(VertexArrayObject& vao, size_t size);
    void render_octree_boxes();
    void render_mesh();
    void render_sensor_rig_boundary();
    void render_tetrahedra();

    // helper functions
    static std::vector<file_loader::vertex> get_cube_vertices(float side_len);
    std::vector<file_loader::vertex> filter_shaded_points(const std::vector<file_loader::vertex>& points);
    bool is_mesh_vertex_cut_distance_ok(int i0, int i1, int i2) const;
    bool is_outside_of_sensor_rig_boundary(int i0, int i1, int i2) const;
    void set_particle_program_uniforms(bool show_non_shaded);
    void randomize_vertex_colors(std::vector<file_loader::vertex>& vertices) const;
    glm::vec3 hsl_to_rgb(float h, float s, float l) const;
    glm::vec3 get_random_color() const;
    glm::vec3 get_sphere_pos(float u, float v) const;
    static void toggle_fullscreen(SDL_Window* win);

    //RANSAC functions
    struct RANSACDiffs {
        int inliersNum;
        std::vector<bool> isInliers;
        std::vector<float> distances;
    };
    float* EstimatePlaneImplicit(const std::vector<file_loader::vertex*>& pts);
    float* EstimatePlaneRANSAC(const std::vector<file_loader::vertex*>& pts, float threshold, int iterNum);
    RANSACDiffs PlanePointRANSACDifferences(const std::vector<file_loader::vertex*>& pts, float* plane, float threshold);
    void RunRANSAC(std::vector<file_loader::vertex>& points, std::vector<std::vector<file_loader::vertex*>>& dest, int iterations);

protected:
    // shader programs
    ProgramObject m_axes_program;
    ProgramObject m_particle_program;
    ProgramObject m_wireframe_program;

    // VAOs
    VertexArrayObject m_particle_vao;
    std::vector<std::shared_ptr<VertexArrayObject>> m_particle_group_vaos;
    VertexArrayObject m_debug_sphere_vao;
    VertexArrayObject m_wireframe_vao;
    VertexArrayObject m_sensor_rig_boundary_vao;
    VertexArrayObject m_tetrahedra_vao;
    VertexArrayObject m_mesh_vao;

    // array buffers
    ArrayBuffer m_particle_buffer;
    std::vector<std::shared_ptr<ArrayBuffer>> m_particle_group_buffers;
    ArrayBuffer m_debug_sphere_buffer;
    ArrayBuffer m_wireframe_vertices_buffer;
    ArrayBuffer m_sensor_rig_boundary_vertices_buffer;
    ArrayBuffer m_tetrahedra_vertices_buffer;
    ArrayBuffer m_mesh_pos_buffer;

    // index buffers
    IndexBuffer m_wireframe_indices_buffer;
    IndexBuffer m_sensor_rig_boundary_indices_buffer;
    IndexBuffer m_tetrahedra_indices_buffer;
    IndexBuffer m_mesh_indices_buffer;

    // index vectors
    std::vector<int> m_wireframe_indices;
    std::vector<int> m_sensor_rig_boundary_indices;
    std::vector<int> m_tetrahedra_indices;
    std::vector<int> m_mesh_indices;

    // vertex vectors
    std::vector<file_loader::vertex> m_vertices;
    std::vector <std::vector<file_loader::vertex*>> m_vertex_groups;
    std::vector<file_loader::vertex> m_delaunay_vertices;
    std::vector<file_loader::vertex> m_wireframe_vertices;
    std::vector<file_loader::vertex> m_sensor_rig_boundary_vertices;
    std::vector<file_loader::vertex> m_tetrahedra_vertices;

    // flags
    std::vector<char> m_show_vertex_groups;
    bool m_show_axes;
    bool m_show_points;
    bool m_show_debug_sphere;
    bool m_show_octree;
    bool m_show_sensor_rig_boundary;
    bool m_show_tetrahedra;
    bool m_show_back_faces;
    bool m_show_non_shaded_points;
    bool m_show_texture;
    bool m_show_non_shaded_mesh;
    bool m_auto_increment_rendered_point_index;

    // numeric values
    int m_render_points_up_to_index;
    int m_debug_sphere_n = 959;
    int m_debug_sphere_m = 959;
    float m_point_size;
    float m_mesh_vertex_cut_distance;
    float m_line_width;

    // other objects
    SDL_Window* m_window{};
    gCamera m_virtual_camera;
    glm::vec3 m_start_eye;
    glm::vec3 m_start_at;
    glm::vec3 m_start_up;
    glm::vec3 m_octree_color;
    octree m_octree;
    delaunay_3d m_delaunay;
    octree::boundary m_sensor_rig_boundary;
    mesh_rendering_mode m_mesh_rendering_mode;
    file_loader::digital_camera_params m_digital_camera_params;
    char m_input_folder[256]{};
    Texture2D m_digital_camera_textures[3];
    std::vector<glm::vec3> m_debug_sphere;

    // ransac
    int m_ransac_object_count = 0;
    float m_ransac_threshold = 0.15f;
    int m_ransac_iter = 500;
};
