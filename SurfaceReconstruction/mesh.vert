#version 460 core

in vec3 vs_in_pos;

out vec3 vs_out_col;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(vs_in_pos, 1);
    vs_out_col = vec3(1);
}
