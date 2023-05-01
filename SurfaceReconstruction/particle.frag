#version 330

in vec3 vs_out_pos;
in vec3 vs_out_col;
in vec2 vs_out_tex[3];

out vec4 fs_out_col;

uniform vec4 color = vec4(1);
uniform sampler2D tex_image[3];

void main()
{
    int sample_count = 0;
    vec4 accumulated_color = vec4(0);

    for (int i = 0; i < 3; i++) {
        if (!any(equal(vs_out_tex[i], vec2(-1, -1)))) {
            if (i == 0) {
                accumulated_color += texture(tex_image[0], vs_out_tex[0]);
            } else if (i == 1) {
                accumulated_color += texture(tex_image[1], vs_out_tex[1]);
            } else {
                accumulated_color += texture(tex_image[2], vs_out_tex[2]);
            }
            sample_count++;
        }
    }

    if (sample_count > 0) {
        fs_out_col = accumulated_color / float(sample_count);
    } else {
        fs_out_col = vec4(vs_out_col, 1);
    }
}
