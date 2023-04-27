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

lidar_mesh application::create_mesh(const std::vector<glm::vec3>& vertices)
{
    lidar_mesh mesh;
    constexpr int N = 936 - 1;
    constexpr int M = 16 - 1;
    GLushort indices[3 * 2 * (N) * (M)];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < M; ++j)
        {
            indices[6 * i + j * 3 * 2 * (N) + 0] = (i + 0) + (j + 0) * (N + 1);
            indices[6 * i + j * 3 * 2 * (N) + 1] = (i + 1) + (j + 0) * (N + 1);
            indices[6 * i + j * 3 * 2 * (N) + 2] = (i + 0) + (j + 1) * (N + 1);
            indices[6 * i + j * 3 * 2 * (N) + 3] = (i + 1) + (j + 0) * (N + 1);
            indices[6 * i + j * 3 * 2 * (N) + 4] = (i + 1) + (j + 1) * (N + 1);
            indices[6 * i + j * 3 * 2 * (N) + 5] = (i + 0) + (j + 1) * (N + 1);
        }
    }

    // Create a vertex array object (VAO) to store the VBOs.
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Create a vertex buffer object (VBO) to store the vertex data.
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    // Create an element buffer object (EBO) to store the index data.
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Specify the vertex attributes for the shader program.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind the VAO.
    glBindVertexArray(0);

    // Store the number of vertices for later rendering.
    mesh.vertex_count = 3 * 2 * (N) * (M);

    return mesh;
}

Tetrahedron application::create_super_tetrahedron(const std::vector<glm::vec3>& point_cloud)
{
    // Compute the AABB of the point cloud.
    glm::vec3 min_point = point_cloud[0];
    glm::vec3 max_point = point_cloud[0];

    for (const auto& point : point_cloud)
    {
        min_point = glm::min(min_point, point);
        max_point = glm::max(max_point, point);
    }

    // Compute the center and size of the AABB.
    glm::vec3 center = (min_point + max_point) * 0.5f;
    glm::vec3 size_vec = max_point - min_point;
    float size = std::max(std::max(size_vec.x, size_vec.y), size_vec.z);

    // Compute the size of the super tetrahedron.
    float super_tetrahedron_size = size * 3.0f;

    // Create the vertices of the super tetrahedron.
    glm::vec3 v1 = center + glm::vec3(0, 0, -super_tetrahedron_size);
    glm::vec3 v2 = center + glm::vec3(0, super_tetrahedron_size * glm::sqrt(2), super_tetrahedron_size);
    glm::vec3 v3 = center + glm::vec3(-super_tetrahedron_size * glm::sqrt(3) / 3, -super_tetrahedron_size * glm::sqrt(2) / 3, super_tetrahedron_size);
    glm::vec3 v4 = center + glm::vec3(super_tetrahedron_size * glm::sqrt(3) / 3, -super_tetrahedron_size * glm::sqrt(2) / 3, super_tetrahedron_size);

    // Create and return the super tetrahedron.
    return Tetrahedron(v1, v2, v3, v4);
}

float length2(const glm::vec3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

/*
bool application::is_point_inside_circumsphere(const glm::vec3& point, const Tetrahedron& tetrahedron)
{
    glm::mat4 m(1.0);
    m[0] = glm::vec4(tetrahedron.vertices[0], glm::dot(tetrahedron.vertices[0], tetrahedron.vertices[0]));
    m[1] = glm::vec4(tetrahedron.vertices[1], glm::dot(tetrahedron.vertices[1], tetrahedron.vertices[1]));
    m[2] = glm::vec4(tetrahedron.vertices[2], glm::dot(tetrahedron.vertices[2], tetrahedron.vertices[2]));
    m[3] = glm::vec4(tetrahedron.vertices[3], glm::dot(tetrahedron.vertices[3], tetrahedron.vertices[3]));

    float determinant = glm::determinant(m);

    glm::mat4 mp = m;
    mp[0] = glm::vec4(point, glm::dot(point, point));
    const float det_p = glm::determinant(mp);

    return (determinant < 0.0f) ? (determinant > det_p) : (determinant < det_p);
}
*/
void circumsphere(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3 v2, const glm::vec3& v3, glm::vec3* center, float* radius)
{
    //Create the rows of our "unrolled" 3x3 matrix
    const glm::vec3 row1 = v1 - v0;
    const float sq_length1 = length2(row1);
    const glm::vec3 row2 = v2 - v0;
    const float sq_length2 = length2(row2);
    const glm::vec3 row3 = v3 - v0;
    const float sq_length3 = length2(row3);

    //Compute the determinant of said matrix
    const float determinant = row1.x * (row2.y * row3.z - row3.y * row2.z)
        - row2.x * (row1.y * row3.z - row3.y * row1.z)
        + row3.x * (row1.y * row2.z - row2.y * row1.z);

    // Compute the volume of the tetrahedron, and precompute a scalar quantity for re-use in the formula
    const float volume = determinant / 6.f;
    const float i_twelve_volume = 1.f / (volume * 12.f);

    center->x = v0.x + i_twelve_volume * ((row2.y * row3.z - row3.y * row2.z) * sq_length1 - (row1.y * row3.z - row3.y * row1.z) * sq_length2 + (row1.y * row2.z - row2.y * row1.z)
        *
        sq_length3);
    center->y = v0.y + i_twelve_volume * (-(row2.x * row3.z - row3.x * row2.z) * sq_length1 + (row1.x * row3.z - row3.x * row1.z) * sq_length2 - (row1.x * row2.z - row2.x * row1.z)
        *
        sq_length3);
    center->z = v0.z + i_twelve_volume * ((row2.x * row3.y - row3.x * row2.y) * sq_length1 - (row1.x * row3.y - row3.x * row1.y) * sq_length2 + (row1.x * row2.y - row2.x * row1.y)
        *
        sq_length3);

    //Once we know the center, the radius is clearly the distance to any vertex
    *radius = glm::length(*center - v0);
}

bool application::is_point_inside_circumsphere(const glm::vec3& point, const Tetrahedron& tetrahedron)
{
    glm::vec3 center;
    float radius;
    circumsphere(tetrahedron.vertices[0], tetrahedron.vertices[1], tetrahedron.vertices[2], tetrahedron.vertices[3], &center, &radius);

    return glm::length(point - center) < radius;
}

/*
bool application::is_point_inside_circumsphere(const glm::vec3& point, const Tetrahedron& tetrahedron)
{
    // Calculate the relative position vectors for each vertex of the tetrahedron
    glm::vec3 v0 = tetrahedron.vertices[0] - point;
    glm::vec3 v1 = tetrahedron.vertices[1] - point;
    glm::vec3 v2 = tetrahedron.vertices[2] - point;
    glm::vec3 v3 = tetrahedron.vertices[3] - point;

    // Compute the squared lengths of the vectors
    float v0_sq = glm::dot(v0, v0);
    float v1_sq = glm::dot(v1, v1);
    float v2_sq = glm::dot(v2, v2);
    float v3_sq = glm::dot(v3, v3);

    // Compute the determinant of the matrix formed by the position vectors and squared lengths
    glm::mat4 matrix(
        v0.x, v0.y, v0.z, v0_sq,
        v1.x, v1.y, v1.z, v1_sq,
        v2.x, v2.y, v2.z, v2_sq,
        v3.x, v3.y, v3.z, v3_sq
    );

    // debug: Print the matrix values
    std::cout << "Matrix values: " << std::endl;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
    // end debug

    float determinant = glm::determinant(matrix);
    std::cout << "Determinant: " << determinant << std::endl;

    // The point is inside the circumsphere if the determinant is positive
    constexpr float EPSILON = 1e-6f;  // Adjust this value
    return determinant > EPSILON;
}
*/
std::vector<Face> application::remove_duplicate_faces(const std::vector<Face>& boundary_faces)
{
    std::vector<Face> unique_faces;
    for (const Face& face : boundary_faces)
    {
        // Sort the vertices in ascending order
        std::array<glm::vec3, 3> sorted_vertices = face.vertices;
        std::sort(sorted_vertices.begin(), sorted_vertices.end(), [](const glm::vec3& a, const glm::vec3& b)
        {
            return a.x < b.x || (a.x == b.x && a.y < b.y) || (a.x == b.x && a.y == b.y && a.z < b.z);
        });

        bool is_duplicate = false;
        for (const Face& unique_face : unique_faces)
        {
            std::array<glm::vec3, 3> unique_sorted_vertices = unique_face.vertices;
            std::sort(unique_sorted_vertices.begin(), unique_sorted_vertices.end(), [](const glm::vec3& a, const glm::vec3& b)
            {
                return a.x < b.x || (a.x == b.x && a.y < b.y) || (a.x == b.x && a.y == b.y && a.z < b.z);
            });

            if (sorted_vertices == unique_sorted_vertices)
            {
                is_duplicate = true;
                break;
            }
        }

        if (!is_duplicate)
        {
            unique_faces.push_back(face);
        }
    }

    return unique_faces;
}

Tetrahedron application::create_tetrahedron(const glm::vec3& point, const Face& face)
{
    Tetrahedron tetrahedron(point, face.vertices[0], face.vertices[1], face.vertices[2]);

    // Compute the circumcenter and circumradius
    glm::mat4 A(
        point.x - face.vertices[0].x, point.y - face.vertices[0].y, point.z - face.vertices[0].z, 0,
        point.x - face.vertices[1].x, point.y - face.vertices[1].y, point.z - face.vertices[1].z, 0,
        point.x - face.vertices[2].x, point.y - face.vertices[2].y, point.z - face.vertices[2].z, 0,
        1, 1, 1, 1
    );

    float detA = glm::determinant(A);
    float s = 1.0f / (2.0f * detA);

    glm::mat4 B(
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[0].x * face.vertices[0].x - face.vertices[0].y * face.vertices[0].y - face.vertices[0].z * face.
        vertices[0].z, point.y - face.vertices[0].y, point.z - face.vertices[0].z, 1,
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[1].x * face.vertices[1].x - face.vertices[1].y * face.vertices[1].y - face.vertices[1].z * face.
        vertices[1].z, point.y - face.vertices[1].y, point.z - face.vertices[1].z, 1,
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[2].x * face.vertices[2].x - face.vertices[2].y * face.vertices[2].y - face.vertices[2].z * face.
        vertices[2].z, point.y - face.vertices[2].y, point.z - face.vertices[2].z, 1,
        1, 1, 1, 1
    );

    glm::mat4 C(
        point.x - face.vertices[0].x,
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[0].x * face.vertices[0].x - face.vertices[0].y * face.vertices[0].y - face.vertices[0].z * face.
        vertices[0].z, point.z - face.vertices[0].z, 1,
        point.x - face.vertices[1].x,
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[1].x * face.vertices[1].x - face.vertices[1].y * face.vertices[1].y - face.vertices[1].z * face.
        vertices[1].z, point.z - face.vertices[1].z, 1,
        point.x - face.vertices[2].x,
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[2].x * face.vertices[2].x - face.vertices[2].y * face.vertices[2].y - face.vertices[2].z * face.
        vertices[2].z, point.z - face.vertices[2].z, 1,
        1, 1, 1, 1
    );

    glm::mat4 D(
        point.x - face.vertices[0].x, point.y - face.vertices[0].y,
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[0].x * face.vertices[0].x - face.vertices[0].y * face.vertices[0].y - face.vertices[0].z * face.
        vertices[0].z, 1,
        point.x - face.vertices[1].x, point.y - face.vertices[1].y,
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[1].x * face.vertices[1].x - face.vertices[1].y * face.vertices[1].y - face.vertices[1].z * face.
        vertices[1].z, 1,
        point.x - face.vertices[2].x, point.y - face.vertices[2].y,
        point.x * point.x + point.y * point.y + point.z * point.z - face.vertices[2].x * face.vertices[2].x - face.vertices[2].y * face.vertices[2].y - face.vertices[2].z * face.
        vertices[2].z, 1,
        1, 1, 1, 1
    );

    float detB = glm::determinant(B);
    float detC = glm::determinant(C);
    float detD = glm::determinant(D);

    tetrahedron.circumcenter = glm::vec3(detB * s, detC * s, detD * s);
    tetrahedron.circumradius = glm::length(point - tetrahedron.circumcenter);

    return tetrahedron;
}

std::vector<Tetrahedron> application::remove_super_tetrahedron_related_tetrahedra(const std::vector<Tetrahedron>& tetrahedra, const Tetrahedron& super_tetrahedron)
{
    std::vector<Tetrahedron> filtered_tetrahedra;
    for (const Tetrahedron& tetrahedron : tetrahedra)
    {
        bool is_related_to_super_tetrahedron = false;
        for (const glm::vec3& super_vertex : super_tetrahedron.vertices)
        {
            for (const glm::vec3& vertex : tetrahedron.vertices)
            {
                if (vertex == super_vertex)
                {
                    is_related_to_super_tetrahedron = true;
                    break;
                }
            }
            if (is_related_to_super_tetrahedron) break;
        }
        if (!is_related_to_super_tetrahedron)
        {
            filtered_tetrahedra.push_back(tetrahedron);
        }
    }
    return filtered_tetrahedra;
}

std::vector<Face> application::extract_mesh_from_tetrahedra(const std::vector<Tetrahedron>& tetrahedra)
{
    std::vector<Face> mesh_faces;

    for (const Tetrahedron& tetrahedron : tetrahedra)
    {
        mesh_faces.push_back(Face(tetrahedron.vertices[0], tetrahedron.vertices[1], tetrahedron.vertices[2]));
        mesh_faces.push_back(Face(tetrahedron.vertices[0], tetrahedron.vertices[1], tetrahedron.vertices[3]));
        mesh_faces.push_back(Face(tetrahedron.vertices[0], tetrahedron.vertices[2], tetrahedron.vertices[3]));
        mesh_faces.push_back(Face(tetrahedron.vertices[1], tetrahedron.vertices[2], tetrahedron.vertices[3]));
    }

    return mesh_faces;
}

std::vector<Face> get_tetrahedron_faces(const Tetrahedron& tetrahedron)
{
    std::vector<Face> faces(4);
    faces[0] = Face(tetrahedron.vertices[0], tetrahedron.vertices[1], tetrahedron.vertices[2]);
    faces[1] = Face(tetrahedron.vertices[0], tetrahedron.vertices[1], tetrahedron.vertices[3]);
    faces[2] = Face(tetrahedron.vertices[0], tetrahedron.vertices[2], tetrahedron.vertices[3]);
    faces[3] = Face(tetrahedron.vertices[1], tetrahedron.vertices[2], tetrahedron.vertices[3]);
    return faces;
}

std::vector<glm::vec3> filter_duplicate_points(const std::vector<glm::vec3>& point_cloud)
{
    std::set<glm::vec3, bool(*)(const glm::vec3&, const glm::vec3&)> unique_points([](const glm::vec3& a, const glm::vec3& b)
    {
        const float epsilon = 1e-6f;
        return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(epsilon))) ? false : a.x < b.x || (a.x == b.x && (a.y < b.y || (a.y == b.y && a.z < b.z)));
    });

    for (const auto& point : point_cloud)
    {
        unique_points.insert(point);
    }

    std::vector<glm::vec3> filtered_point_cloud(unique_points.begin(), unique_points.end());
    return filtered_point_cloud;
}


std::vector<Face> application::delaunay_triangulation_3d(const std::vector<glm::vec3>& point_cloud)
{
    const Tetrahedron super_tetrahedron = create_super_tetrahedron(point_cloud);
    std::vector<Tetrahedron> tetrahedra;
    tetrahedra.push_back(super_tetrahedron);
    for (const glm::vec3& point : point_cloud)
    {
        std::vector<Tetrahedron> tetrahedra_to_remove;
        std::vector<Face> boundary_faces;
        for (const Tetrahedron& tetrahedron : tetrahedra)
        {
            if (is_point_inside_circumsphere(point, tetrahedron))
            {
                tetrahedra_to_remove.push_back(tetrahedron);
                std::vector<Face> tetrahedron_faces = get_tetrahedron_faces(tetrahedron);
                boundary_faces.insert(boundary_faces.end(), tetrahedron_faces.begin(), tetrahedron_faces.end());
            }
        }
        std::vector<Face> unique_boundary_faces = remove_duplicate_faces(boundary_faces);

        std::cout << "After Step 4.6 boundary_faces size: " << boundary_faces.size() << std::endl;
        std::cout << "After Step 4.6 unique_boundary_faces size: " << unique_boundary_faces.size() << std::endl;

        for (const Tetrahedron& tetrahedron : tetrahedra_to_remove)
        {
            tetrahedra.erase(std::remove(tetrahedra.begin(), tetrahedra.end(), tetrahedron), tetrahedra.end());
        }

        std::cout << "After Step 4.7 tetrahedra size: " << tetrahedra.size() << std::endl;

        for (const Face& face : unique_boundary_faces)
        {
            Tetrahedron new_tetrahedron = create_tetrahedron(point, face);
            tetrahedra.push_back(new_tetrahedron);
        }

        std::cout << "After Step 4.8 tetrahedra size: " << tetrahedra.size() << std::endl;
    }
    tetrahedra = remove_super_tetrahedron_related_tetrahedra(tetrahedra, super_tetrahedron);
    std::vector<Face> triangulation = extract_mesh_from_tetrahedra(tetrahedra);
    return triangulation;
}

std::vector<glm::vec3> application::normalize_point_cloud(const std::vector<glm::vec3>& point_cloud)
{
    // Calculate the centroid (mean) of the point cloud
    glm::vec3 centroid(0, 0, 0);
    for (const auto& point : point_cloud)
    {
        centroid += point;
    }
    centroid /= static_cast<float>(point_cloud.size());

    // Translate the point cloud to the origin by subtracting the centroid
    std::vector<glm::vec3> translated_point_cloud;
    translated_point_cloud.reserve(point_cloud.size());
    for (const auto& point : point_cloud)
    {
        translated_point_cloud.push_back(point - centroid);
    }

    // Calculate the scale factor to fit the point cloud within a reasonable bounding box
    float max_distance = 0;
    for (const auto& point : translated_point_cloud)
    {
        max_distance = std::max(max_distance, glm::length(point));
    }
    float scale_factor = 1 / max_distance;

    // Scale the point cloud by the scale factor
    std::vector<glm::vec3> normalized_point_cloud;
    normalized_point_cloud.reserve(translated_point_cloud.size());
    for (const auto& point : translated_point_cloud)
    {
        normalized_point_cloud.push_back(point * scale_factor);
    }

    return normalized_point_cloud;
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
        file >> vertices.positions[i].x >> vertices.positions[i].y >> vertices.positions[i].z;
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
    glLineWidth(4.0f);
    glPointSize(5.0f);

    m_axes_program.Init({{GL_VERTEX_SHADER, "axes.vert"}, {GL_FRAGMENT_SHADER, "axes.frag"}});
    m_particle_program.Init({{GL_VERTEX_SHADER, "particle.vert"}, {GL_FRAGMENT_SHADER, "particle.frag"}}, {{0, "vs_in_pos"}, {1, "vs_in_tex"}});
    m_mesh_program.Init({{GL_VERTEX_SHADER, "mesh.vert"}, {GL_FRAGMENT_SHADER, "mesh.frag"}}, {{0, "vs_in_pos"}});

    m_camera_texture.FromFile("inputs/garazs_kijarat/Dev0_Image_w960_h600_fn644.jpg");
    // m_camera_texture.FromFile("inputs/debug.png");

    m_camera_params = load_camera_params("inputs/CameraParameters_minimal.txt");
    // m_vertices = load_xyz_file("inputs/garazs_kijarat/test_fn644.xyz");

    reset();

    constexpr int n = 4;
    constexpr int m = 4;
    m_debug_sphere.resize((m + 1) * (n + 1));
    for (int i = 0; i <= n; ++i)
        for (int j = 0; j <= m; ++j)
            m_debug_sphere[i + j * (n + 1)] = get_sphere_pos(static_cast<float>(i) / static_cast<float>(n), static_cast<float>(j) / static_cast<float>(m));
    m_gpu_debug_sphere_buffer.BufferData(m_debug_sphere);
    m_gpu_debug_sphere_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_debug_sphere_buffer}});

    // m_gpu_particle_buffer.BufferData(m_vertices.positions);
    // m_gpu_particle_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_particle_buffer}});

    // m_lidar_mesh = create_mesh(m_vertices.positions);

    std::vector<Face> faces = delaunay_triangulation_3d(filter_duplicate_points(m_debug_sphere));
    //std::vector<Face> faces = delaunay_triangulation_3d(normalize_point_cloud(m_vertices.positions));
    m_triangle_mesh = create_mesh_from_faces(faces);

    // glGenVertexArrays(1, &mesh_vao);
    // glGenBuffers(1, &mesh_vbo);
    // glGenBuffers(1, &mesh_ebo);
    //
    // glBindVertexArray(mesh_vao);
    //
    // glBindBuffer(GL_ARRAY_BUFFER, mesh_vbo);
    // glBufferData(GL_ARRAY_BUFFER, m_triangle_mesh.vertices.size() * sizeof(glm::vec3), m_triangle_mesh.vertices.data(), GL_STATIC_DRAW);
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    //
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_ebo);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_triangle_mesh.indices.size() * sizeof(GLuint), m_triangle_mesh.indices.data(), GL_STATIC_DRAW);
    //
    // glBindVertexArray(0);

    m_gpu_buffer_pos.BufferData(m_triangle_mesh.vertices);
    m_gpu_buffer_indices.BufferData(m_triangle_mesh.indices);
    m_vao.Init({{CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, m_gpu_buffer_pos},}, m_gpu_buffer_indices);

    return true;
}

void application::clean()
{
    // glDeleteVertexArrays(1, &mesh_vao);
    // glDeleteBuffers(1, &mesh_vbo);
    // glDeleteBuffers(1, &mesh_ebo);
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
    // m_gpu_particle_buffer.BufferData(m_vertices.positions);
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
    program.SetTexture("texImage", 0, texture);
    glDrawArrays(GL_POINTS, 0, size);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

triangle_mesh application::create_mesh_from_faces(const std::vector<Face>& faces)
{
    triangle_mesh mesh;
    for (const auto& face : faces)
    {
        mesh.vertices.push_back(face.vertices[0]);
        mesh.vertices.push_back(face.vertices[1]);
        mesh.vertices.push_back(face.vertices[2]);

        GLuint currentIndex = static_cast<GLuint>(mesh.vertices.size()) - 3;
        mesh.indices.push_back(currentIndex);
        mesh.indices.push_back(currentIndex + 1);
        mesh.indices.push_back(currentIndex + 2);
    }
    return mesh;
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

    //m_axes_program.Use();
    //m_axes_program.SetUniform("mvp", mvp);
    //glDrawArrays(GL_LINES, 0, 6);

    //draw_points(mvp, world, m_gpu_particle_vao, m_particle_program, m_vertices.positions.size(), m_camera_params, m_camera_texture);
    //draw_points(mvp, world, m_gpu_debug_sphere_vao, m_particle_program, m_debug_sphere.size(), m_camera_params, m_camera_texture);

    // m_mesh_program.Use();
    // m_mesh_program.SetUniform("mvp", mvp);
    // glBindVertexArray(mesh_vao);
    // glDrawElements(GL_TRIANGLES, m_triangle_mesh.indices.size(), GL_UNSIGNED_INT, nullptr);
    // glBindVertexArray(0);

    m_vao.Bind();

    m_mesh_program.Use();
    m_mesh_program.SetUniform("mvp", mvp);

    glDrawElements(GL_TRIANGLES, m_triangle_mesh.indices.size(), GL_UNSIGNED_INT, nullptr);

    /*m_mesh_program.Use();
    m_mesh_program.SetUniform("mvp", mvp);
    glBindVertexArray(triangle_mesh.vao);
    glDrawElements(GL_TRIANGLES, triangle_mesh.vertex_count, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);*/

    // if (ImGui::Begin("Points"))
    // {
    //     ImGui::Text("Properties");
    // }
    // ImGui::End();
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
