#version 330

vec4 positions[6] = vec4[6](
    // 1. szakasz
    vec4(0, 0, 0, 1),
    vec4(5, 0, 0, 1),
    // 2. szakasz
    vec4(0, 0, 0, 1),
    vec4(0, 5, 0, 1),
    // 3. szakasz
    vec4(0, 0, 0, 1),
    vec4(0, 0, 5, 1)
);

vec4 colors[3] = vec4[3](
    vec4(1, 0, 0, 1),
    vec4(0, 1, 0, 1),
    vec4(0, 0, 1, 1)
);

out vec4 vs_out_color;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * positions[gl_VertexID];
    vs_out_color = colors[gl_VertexID/2];
}
