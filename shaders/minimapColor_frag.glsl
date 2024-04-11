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
layout(location = 24) uniform sampler2D texSpotlight;
layout(location = 25) uniform bool useSpotlight = false;
layout(location = 26) uniform vec3 spotlightPos = vec3(0.0);
layout(location = 27) uniform mat4 lightMVP = mat4(1.0);
layout(location = 29) uniform bool isNight = false;

// Texture uniforms
layout(location = 30) uniform sampler2D texColor;
layout(location = 31) uniform sampler2D texNormal;
layout(location = 32) uniform bool useNormalMap = false;

// Env map
layout(location = 60) uniform samplerCube cubemap;

in vec3 fragPos;
in vec3 fragNormal;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(forceColor, 1.0);
}
