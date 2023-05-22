#version 330

in vec3 vs_out_pos;
in vec3 vs_out_col;

out vec4 fs_out_color;

void main()
{
    fs_out_color = vec4(vs_out_col, 1.0);
}
