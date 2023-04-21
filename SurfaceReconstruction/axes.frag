#version 460 core

in vec4 vs_out_color;

out vec4 fs_out_col;

void main()
{
    fs_out_col = vs_out_color;
}
