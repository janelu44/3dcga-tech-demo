#version 450

layout(location = 3) uniform sampler2D texColor;
layout(location = 4) uniform bool hasTexCoords;
layout(location = 5) uniform vec3 viewPos = vec3(0.0);
layout(location = 6) uniform vec3 lightPos = vec3(0.0);
layout(location = 7) uniform vec3 forceColor = vec3(1.0);
layout(location = 8) uniform bool ignoreBehind = false;
layout(location = 9) uniform bool useShadow = false;
layout(location = 10) uniform samplerCube texShadow;

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragKd;
in vec3 fragKs;
in float fragShininess;
in float fragRoughness;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

float lambert(bool ignoreBehind) {
    float l = dot(fragNormal, normalize(lightPos - fragPos));
    if (ignoreBehind) return abs(l);
    return max(l, 0.0);
}

//float blinnPhong(bool ignoreBehind) {
//    vec3 H = normalize(viewPos - fragPos + lightPos - fragPos);
//    vec3 N = normalize(fragNormal);
//    float d = dot(H, N);
//    if (ignoreBehind) d = abs(d);
//    if (dot(lightPos - fragPos, fragNormal) <= 0.0) {
//        d = 0.0;
//    }
//    return pow(d, fragShininess);
//}

vec3 sampleOffsetDirections[20] = vec3[] (
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float computeShadow() {
    const vec3 lightVec = fragPos - lightPos;
    const float viewDistance = length(viewPos - fragPos);

    float shadow = 0.0;
    float bias = 0.15 * (1.0 - dot(fragNormal, normalize(lightVec)));
    int pcfSamples = 20;
    float diskRadius = 0.001 + pow(viewDistance, 1.1) * 0.001;
//    float diskRadius = 0.001;

    for(int i = 0; i < pcfSamples; ++i) {
        vec3 sampleVec = lightVec + sampleOffsetDirections[i] * diskRadius;
        float sampleDistance = texture(texShadow, normalize(sampleVec)).x;
        if(length(lightVec) < sampleDistance + bias) shadow += 1.0;
    }

    return shadow / float(pcfSamples);
}

void main() {
    const vec3 normal = normalize(fragNormal);

    vec3 color = vec3(0.0);
    if (hasTexCoords) color = texture(texColor, fragTexCoord).rgb;
    else color = lambert(ignoreBehind) * forceColor + forceColor * 0.1f;

    float shadow = useShadow ? computeShadow() : 1.0;

    fragColor = vec4(color * shadow, 1.0);
}
