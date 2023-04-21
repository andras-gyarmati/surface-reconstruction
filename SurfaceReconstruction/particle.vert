#version 460 core

in vec3 vs_in_pos;
in vec2 vs_in_tex;

out vec3 vs_out_pos;
out vec2 vs_out_tex;

uniform mat4 mvp;
uniform mat4 world;

uniform vec3 cam_r_0;
uniform vec3 cam_r_1;
uniform vec3 cam_r_2;
uniform vec3 cam_t;
uniform vec3 cam_k_0;
uniform vec3 cam_k_1;
uniform vec3 cam_k_2;

void main()
{
    mat3 cam_r = mat3(
    cam_r_0[0], cam_r_0[1], cam_r_0[2],
    cam_r_1[0], cam_r_1[1], cam_r_1[2],
    cam_r_2[0], cam_r_2[1], cam_r_2[2]);

    mat3 cam_k = mat3(
    cam_k_0[0], cam_k_0[1], cam_k_0[2],
    cam_k_1[0], cam_k_1[1], cam_k_1[2],
    cam_k_2[0], cam_k_2[1], cam_k_2[2]);

    gl_PointSize = 2;
    gl_Position = mvp * vec4(vs_in_pos, 1);

    vs_out_pos  = (world * vec4(vs_in_pos, 1)).xyz;

    vec3 p_c = ((cam_r * vs_out_pos + cam_t) * cam_k);
    p_c = p_c / p_c.z;

    if (p_c.z >= 0) {
        vs_out_tex = vec2(p_c.x / 960.0f, p_c.y / 600.0f);
    } else {
        vs_out_tex = vec2(-1, -1);
    }

//    if (vs_out_pos.z >= 0) {
//        vs_out_tex = vs_out_pos.xy;
//    } else {
//        vs_out_tex = vec2(-1, -1);
//    }
}
