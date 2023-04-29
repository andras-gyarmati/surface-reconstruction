#version 330

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

    gl_PointSize = 5;
    gl_Position = mvp * vec4(vs_in_pos, 1);

    vs_out_pos = (world * vec4(vs_in_pos, 1)).xyz;

    vec3 p_tmp = ((cam_r) * (vs_in_pos - cam_t));
    float dist = p_tmp.z;
    p_tmp /= p_tmp.z;
    vec2 p_c;
    p_c.x = cam_k_0[0] * p_tmp.x + cam_k_0[2];
    p_c.y = cam_k_1[1] * -p_tmp.y + cam_k_1[2];
    if (dist > 0 && p_c.x >= 0 && p_c.x <= 960 && p_c.y >= 0 && p_c.y <= 600)
    {
        vs_out_tex = vec2(p_c.x / 960.0f, p_c.y / 600.0f);
    }
    else
    {
        vs_out_tex = vec2(-1, -1);
    }
}
