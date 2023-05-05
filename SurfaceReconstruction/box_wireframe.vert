#version 330

in vec3 vs_in_pos;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(vs_in_pos, 1);
}
