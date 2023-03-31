#version 330 core

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

    //    mat3 cam_r = mat3(
    //    0.9230, -0.0066, -0.3847,
    //    0.3848, 0.0203, 0.9228,
    //    0.0018, -0.9998, 0.0213);
    //
    //    mat3 cam_k = mat3(
    //    625.0, 0.0, 480.0,
    //    0.0, 625.0, 300.0,
    //    0.0, 0.0, 1.0);
    //
    //    mat4 t_cl = mat4(
    //    0.9230, -0.0066, -0.3847, -0.0397,
    //    0.3848, 0.0203, 0.9228, 0.1842,
    //    0.0018, -0.9998, 0.0213, -0.0944,
    //    0.0, 0.0, 0.0, 1.0);

    gl_PointSize = 5;
    gl_Position = mvp * vec4(vs_in_pos, 1);

    vs_out_pos  = (world * vec4(vs_in_pos, 1)).xyz;

    // todo: calc vs_out_tex based on camera params
    // vs_out_tex = ((cam_r * vs_out_pos + cam_t) * cam_k).xy;
    // todo: if z is negative we skip it
    // todo: refactor cam_k inverse
    vec3 p_c = cam_r * vs_out_pos + cam_t;
    float x = (p_c.x/p_c.z - cam_k[0][2]) / cam_k[0][0];
    float y = (p_c.y/p_c.z - cam_k[1][2]) / cam_k[1][1];
    vs_out_tex = vec2(x, y);
}
