#include "parametric_surface.h"

glm::vec3 parametric_surface::to_descartes(const float fi, const float theta) {
    return {
        sinf(theta) * cosf(fi),
        cosf(theta),
        sinf(theta) * sinf(fi)
    };
}

glm::vec3 parametric_surface::get_sphere_pos(const float u, const float v) {
    const float th = u * 2.0f * static_cast<float>(M_PI);
    const float fi = v * 2.0f * static_cast<float>(M_PI);
    constexpr float r = 2.0f;

    return {
        r * sin(th) * cos(fi),
        r * sin(th) * sin(fi),
        r * cos(th)
    };
}
