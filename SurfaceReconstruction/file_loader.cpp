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

#include <glm/gtc/type_ptr.hpp>

#include "file_loader.h"

namespace file_loader {
    physical_camera_params load_physical_camera_params(const std::string& filename) {
        physical_camera_params camera_params;

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

    std::vector<vertex> read_vertices_from_file(std::ifstream* file, const int num_vertices) {
        std::vector<vertex> vertices;
        vertices.resize(num_vertices);

        for (int i = 0; i < num_vertices; ++i) {
            *file >> vertices[i].position.x >> vertices[i].position.y >> vertices[i].position.z;
            *file >> vertices[i].color.r >> vertices[i].color.g >> vertices[i].color.b;
            if (i % 2 == 0)
                vertices[i].color = {1, 0, 0};
        }
        file->close();

        return vertices;
    }

    std::vector<vertex> load_ply_file(const std::string& filename) {
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

    std::vector<vertex> load_xyz_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Could not open file: " << filename << std::endl;
            return {};
        }

        const int num_vertices = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');

        file.seekg(0, std::ios::beg);

        return read_vertices_from_file(&file, num_vertices);
    }
}
