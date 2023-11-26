#include <vector>
#include <array>
#include <random>
#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "file_loader.h"

file_loader::digital_camera_params file_loader::load_digital_camera_params(const std::string& filename) {
    digital_camera_params camera_params;

    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return camera_params;
    }

    file >> camera_params.internal_params.fu
        >> camera_params.internal_params.u0
        >> camera_params.internal_params.fv
        >> camera_params.internal_params.v0;

    int num_devices;
    file >> num_devices;

    for (int i = 0; i < num_devices; ++i) {
        device dev;

        file >> dev.name;

        glm::mat3& r = dev.r;
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                file >> r[j][k];
            }
        }

        for (int j = 0; j < 3; ++j) {
            float t_val;
            file >> t_val;
            dev.t[j] = t_val;
        }

        camera_params.devices.push_back(dev);
    }

    return camera_params;
}

std::vector<file_loader::vertex> file_loader::read_vertices_from_file(std::ifstream* file, const int vertex_count) {
    std::vector<vertex> vertices;
    vertices.resize(vertex_count);

    for (int i = 0; i < vertex_count; ++i) {
        *file >> vertices[i].position.x >> vertices[i].position.y >> vertices[i].position.z;
        *file >> vertices[i].color.r >> vertices[i].color.g >> vertices[i].color.b;
        vertices[i].color = {0, 0, 0};
    }
    file->close();

    return vertices;
}

std::vector<file_loader::vertex> file_loader::load_ply_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return {};
    }

    std::string line;
    int num_vertices = 0;
    bool header_finished = false;
    while (std::getline(file, line)) {
        if (line == "end_header") {
            header_finished = true;
            break;
        }

        if (line.find("element vertex") == 0) {
            std::sscanf(line.c_str(), "element vertex %d", &num_vertices);
        }
    }

    if (!header_finished) {
        std::cerr << "Error: Could not find end_header in PLY file" << std::endl;
        return {};
    }

    return read_vertices_from_file(&file, num_vertices);
}

std::vector<file_loader::vertex> file_loader::load_xyz_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return {};
    }

    const int num_vertices = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');

    file.seekg(0, std::ios::beg);

    auto vertices = read_vertices_from_file(&file, num_vertices);

    bool delete_next = false;
    for (auto it = vertices.begin(); it != vertices.end();) {
        if (delete_next /*|| it->position == glm::vec3(0, 0, 0)*/) {
            it = vertices.erase(it);
        } else {
            ++it;
        }
        delete_next = !delete_next;
    }

    const int reorder_indices[192] = {
        11, 35, 59, 83, 107, 131, 155, 179, 23, 47, 71, 95, 119, 143, 167, 191,
        10, 34, 58, 82, 106, 130, 154, 178, 22, 46, 70, 94, 118, 142, 166, 190,
        9, 33, 57, 81, 105, 129, 153, 177, 21, 45, 69, 93, 117, 141, 165, 189,
        8, 32, 56, 80, 104, 128, 152, 176, 20, 44, 68, 92, 116, 140, 164, 188,
        7, 31, 55, 79, 103, 127, 151, 175, 19, 43, 67, 91, 115, 139, 163, 187,
        6, 30, 54, 78, 102, 126, 150, 174, 18, 42, 66, 90, 114, 138, 162, 186,
        5, 29, 53, 77, 101, 125, 149, 173, 17, 41, 65, 89, 113, 137, 161, 185,
        4, 28, 52, 76, 100, 124, 148, 172, 16, 40, 64, 88, 112, 136, 160, 184,
        3, 27, 51, 75, 99, 123, 147, 171, 15, 39, 63, 87, 111, 135, 159, 183,
        2, 26, 50, 74, 98, 122, 146, 170, 14, 38, 62, 86, 110, 134, 158, 182,
        1, 25, 49, 73, 97, 121, 145, 169, 13, 37, 61, 85, 109, 133, 157, 181,
        0, 24, 48, 72, 96, 120, 144, 168, 12, 36, 60, 84, 108, 132, 156, 180
    };

    std::vector<vertex> reordered_vertices = std::vector<vertex>(vertices.size());
    for (int i = 0; i < vertices.size(); ++i) {
        reordered_vertices[i] = vertices[reorder_indices[i % 192] + (i / 192) * 192];
    }

    return reordered_vertices;
}

std::vector<std::string> file_loader::get_directory_files(const std::string& folder_name) {
    std::vector<std::string> file_paths;
    for (const auto& entry : std::filesystem::directory_iterator(folder_name)) {
        file_paths.push_back(entry.path().string());
    }
    return file_paths;
}
