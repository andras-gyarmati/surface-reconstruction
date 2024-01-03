#version 330

in vec3 vs_out_pos;
in vec3 vs_out_col;
in vec3 vs_out_norm;
in vec2 vs_out_tex[3];

out vec4 fs_out_col;

uniform vec4 color = vec4(1);
uniform sampler2D tex_image[3];
uniform int show_non_shaded;
uniform int show_normal;
uniform int show_color;

void main()
{
    if (show_normal == 1) {
        fs_out_col = vec4(vs_out_norm * 0.5 + 0.5, 1);
        return;
    }

    if (show_color == 1) {
        fs_out_col = vec4(vs_out_col, 1);
        return;
    }

    if (!any(equal(vs_out_tex[1], vec2(-1, -1)))) {
        fs_out_col = texture(tex_image[1], vs_out_tex[1]);
    } else if (!any(equal(vs_out_tex[0], vec2(-1, -1)))) {
        fs_out_col = texture(tex_image[0], vs_out_tex[0]);
    } else if (!any(equal(vs_out_tex[2], vec2(-1, -1)))) {
        fs_out_col = texture(tex_image[2], vs_out_tex[2]);
    } else if (show_non_shaded == 1) {
        fs_out_col = vec4(vs_out_col, 1);
    } else {
        discard;
    }
}
