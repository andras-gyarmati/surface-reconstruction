#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <string>

class file_loader {
public:
    struct vertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    struct digital_camera_internal_params {
        float fu, fv, u0, v0;
    };

    struct device {
        std::string name;
        glm::mat3 r;
        glm::vec3 t;
    };

    struct digital_camera_params {
        digital_camera_internal_params internal_params{};
        std::vector<device> devices;

        glm::mat3 get_cam_k() {
            return {
                internal_params.fu, 0.0f, internal_params.u0,
                0.0f, internal_params.fv, internal_params.v0,
                0.0f, 0.0f, 1.0f
            };
        }
    };

    static digital_camera_params load_digital_camera_params(const std::string& filename);
    static std::vector<vertex> read_vertices_from_file(std::ifstream* file, int num_vertices);
    static std::vector<vertex> load_ply_file(const std::string& filename);
    static std::vector<vertex> load_xyz_file(const std::string& filename);
    static std::vector<std::string> get_directory_files(const std::string& folder_name);
};
