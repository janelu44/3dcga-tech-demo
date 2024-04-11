#version 450

layout(location = 5) uniform vec3 viewPos = vec3(0.0);
layout(location = 6) uniform vec3 lightPos = vec3(0.0);
layout(location = 7) uniform vec3 forceColor = vec3(1.0);
layout(location = 8) uniform bool ignoreBehind = false;

// Shadow uniforms
layout(location = 20) uniform bool useShadow = false;
layout(location = 21) uniform samplerCube texShadow;
layout(location = 22) uniform float baseBias = 0.15;
layout(location = 23) uniform float baseDisk = 0.001;
layout(location = 25) uniform bool isNight = false;

// Texture uniforms
layout(location = 30) uniform sampler2D texColor;
layout(location = 31) uniform sampler2D texNormal;
layout(location = 32) uniform bool useNormalMap = false;
layout(location = 33) uniform sampler2D texAlbedo;
layout(location = 34) uniform bool useAlbedoMap = false;
layout(location = 35) uniform sampler2D texMetallic;
layout(location = 36) uniform bool useMetallicMap = false;
layout(location = 37) uniform sampler2D texRoughness;
layout(location = 38) uniform bool useRoughnessMap = false;
layout(location = 39) uniform sampler2D texAo;
layout(location = 40) uniform bool useAoMap = false;

// Env map
layout(location = 60) uniform samplerCube cubemap;

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragKd;
in vec3 fragKs;
in float fragShininess;
in float fragRoughness;
in vec2 fragTexCoord;
in vec3 fragCubemapCoord;

layout(location = 0) out vec4 fragColor;

void _reflect() {
    vec3 I = normalize(fragPos - viewPos);
    vec3 R = reflect(I, normalize(fragNormal));
    fragColor = texture(cubemap, normalize(R)) + vec4(0.05f);
}

void _refract() {
    float ratio = 1.00 / 1.52;
    vec3 I = normalize(fragPos - viewPos);
    vec3 R = refract(I, normalize(fragNormal), ratio);
    fragColor = vec4(texture(cubemap, R).rgb, 1.0);
}

void main() {
    _reflect();
}