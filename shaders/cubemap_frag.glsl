#version 450

layout(location = 70) uniform samplerCube cubemap;

in vec3 fragPos;
in vec3 fragCubemapCoord;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(cubemap, fragCubemapCoord);
//    fragColor = vec4(fragPos * 0.5f + 0.5f, 1.0);
}
