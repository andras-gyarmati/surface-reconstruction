#version 330

in vec3 vs_in_pos;
in vec3 vs_in_col;
in vec3 vs_in_ransac;
in vec3 vs_in_norm;
in vec3 vs_in_uv_stretch;
in vec3 vs_in_bfs_col;

out vec3 vs_out_pos;
out vec3 vs_out_col;
out vec3 vs_out_ransac;
out vec3 vs_out_norm;
out vec3 vs_out_uv_stretch;
out vec3 vs_out_bfs_col;
out vec2 vs_out_tex[3];

uniform mat4 mvp;
uniform mat4 world;

uniform mat3 cam_r[3];
uniform vec3 cam_t[3];
uniform mat3 cam_k;
uniform float point_size;

void main()
{
    vs_out_col = vs_in_col;
    vs_out_ransac = vs_in_ransac;
    vs_out_norm = vs_in_norm;
    vs_out_uv_stretch = vs_in_uv_stretch;
    vs_out_bfs_col = vs_in_bfs_col;
    gl_PointSize = point_size;
    gl_Position = mvp * vec4(vs_in_pos, 1);

    vs_out_pos = (world * vec4(vs_in_pos, 1)).xyz;

    for (int i = 0; i < 3; ++i)
    {
        // lidar to camera transform
        vec3 p_tmp = cam_r[i] * (vs_in_pos - cam_t[i]);
        float dist = p_tmp.z;
        p_tmp /= p_tmp.z;
        vec2 p_c;
        // get the uv coordinate
        p_c.x = cam_k[0][0] * p_tmp.x + cam_k[0][2];
        p_c.y = cam_k[1][1] * -p_tmp.y + cam_k[1][2];
        if (dist > 0 && p_c.x >= 0 && p_c.x <= 960 && p_c.y >= 0 && p_c.y <= 600)
        {
            vs_out_tex[i] = vec2(p_c.x / 960.0f, p_c.y / 600.0f);
        }
        else
        {
            vs_out_tex[i] = vec2(-1, -1);
        }
    }
}
