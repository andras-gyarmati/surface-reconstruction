#pragma once
#include <glm/glm.hpp>
#include <SDL.h>

class parametric_surface {
public:
    static glm::vec3 to_descartes(float fi, float theta);
    static glm::vec3 get_sphere_pos(float u, float v);
};
