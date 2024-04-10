#version 450

layout(location = 5) uniform vec3 viewPos = vec3(0.0);
layout(location = 6) uniform vec3 lightPos = vec3(0.0);
layout(location = 7) uniform vec3 forceColor = vec3(1.0);
layout(location = 8) uniform bool ignoreBehind = false;

// Shadow uniforms
layout(location = 20) uniform bool useShadow = false;
layout(location = 21) uniform samplerCube texShadow;
layout(location = 22) uniform float baseBias = 0.15;

// Texture uniforms
layout(location = 30) uniform sampler2D texColor;
layout(location = 31) uniform sampler2D texNormal;
layout(location = 32) uniform bool useNormalMap = false;

uniform samplerCube cubemap;

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