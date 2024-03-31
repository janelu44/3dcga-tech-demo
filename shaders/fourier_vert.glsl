#version 450 core

out vec2 coord;

void main() {
    vec2 vertices[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));

    gl_Position = vec4(vertices[gl_VertexID], 0, 1);
    coord = (gl_Position.xy + 1.0) * 5.0 / 2.0 - 0.5;
}