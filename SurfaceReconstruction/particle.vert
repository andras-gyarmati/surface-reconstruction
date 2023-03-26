#version 330 core

in vec3 vs_in_pos;
in vec2 vs_in_tex;

out vec3 vs_out_pos;
out vec2 vs_out_tex;

uniform mat4 mvp;
uniform mat4 world;

void main()
{
    gl_PointSize = 2;
    gl_Position = mvp * vec4(vs_in_pos, 1);

    vs_out_pos  = (world * vec4(vs_in_pos, 1)).xyz;

    // todo: calc vs_out_tex based on camera params
    vs_out_tex = vs_in_tex;
}
