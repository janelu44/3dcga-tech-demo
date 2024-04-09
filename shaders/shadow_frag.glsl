#version 450

layout(location = 3) uniform sampler2D texColor;
layout(location = 4) uniform bool hasTexCoords;
layout(location = 5) uniform vec3 viewPos = vec3(0.0);
layout(location = 6) uniform vec3 lightPos;
layout(location = 7) uniform vec3 forceColor = vec3(1.0);
layout(location = 8) uniform bool ignoreBehind = false;
layout(location = 9) uniform sampler2D texNormal;

// Shadow uniforms
layout(location = 20) uniform bool useShadow = false;
layout(location = 21) uniform samplerCube texShadow;
layout(location = 22) uniform float baseBias = 0.15;

in vec3 fragPos;

layout(location = 0) out float fragColor;

void main()
{
    fragColor = length(lightPos - fragPos);
}