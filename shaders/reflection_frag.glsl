#version 450

uniform samplerCube cubemap;
layout(location = 3) uniform sampler2D texColor;
layout(location = 4) uniform bool hasTexCoords;
layout(location = 5) uniform vec3 viewPos = vec3(0.0);
layout(location = 6) uniform vec3 lightPos;
layout(location = 7) uniform vec3 forceColor = vec3(1.0);
layout(location = 8) uniform bool ignoreBehind = false;

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragKd;
in vec3 fragKs;
in float fragShininess;
in float fragRoughness;
in vec2 fragTexCoord;
in vec3 fragCubemapCoord;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 I = normalize(fragPos - viewPos);
    vec3 R = reflect(I, normalize(fragNormal));
    fragColor = texture(cubemap, normalize(R));
}