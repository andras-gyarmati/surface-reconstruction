#version 330

in vec3 vs_out_pos;
in vec2 vs_out_tex;

out vec4 fs_out_col;

uniform vec4 color = vec4(1);
uniform sampler2D tex_image;

void main()
{
    fs_out_col = texture(tex_image, vs_out_tex);
}
