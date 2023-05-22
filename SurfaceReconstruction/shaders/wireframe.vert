#version 330

in vec3 vs_in_pos;
in vec3 vs_in_col;

out vec3 vs_out_pos;
out vec3 vs_out_col;

uniform mat4 mvp;

void main()
{
    vs_out_col = vs_in_col;
    gl_Position = mvp * vec4(vs_in_pos, 1);
    vs_out_pos = normalize(vs_in_pos);
}
