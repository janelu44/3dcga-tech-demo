#version 450

layout(location = 5) uniform vec3 viewPos = vec3(0.0);
layout(location = 6) uniform vec3 lightPos = vec3(0.0);
layout(location = 7) uniform vec3 forceColor = vec3(1.0);
layout(location = 8) uniform bool ignoreBehind = false;

// Shadow uniforms
layout(location = 20) uniform bool useShadow = false;
layout(location = 21) uniform samplerCube texShadow;
layout(location = 22) uniform float baseBias = 0.15;
layout(location = 25) uniform bool isNight = false;

// Texture uniforms
layout(location = 30) uniform sampler2D texColor;
layout(location = 31) uniform sampler2D texNormal;
layout(location = 32) uniform bool useNormalMap = false;

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragTangent;
in vec3 fragBiTangent;
in vec3 fragKd;
in vec3 fragKs;
in float fragShininess;
in float fragRoughness;
in vec2 fragTexCoord;
in vec3 fragCubemapCoord;

layout(location = 0) out vec4 fragColor;

float lambert(vec3 normal) {
    float l = dot(normal, normalize(lightPos - fragPos));
    if (ignoreBehind) return abs(l);
    return max(l, 0.0);
}

float lambertWithNormal(mat3 tbn, vec2 texCoord) {
    vec3 normal = texture(texNormal, texCoord).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(tbn * normal);
    return lambert(normal);
}

float blinnPhong() {
    vec3 H = normalize(viewPos - fragPos + lightPos - fragPos);
    vec3 N = normalize(fragNormal);
    float d = dot(H, N);
    if (dot(lightPos - fragPos, fragNormal) <= 0.0) {
        d = 0.0;
    }
    return pow(d, fragShininess);
}

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
    float bias = baseBias * (1.0 - dot(fragNormal, normalize(lightVec)));
    int pcfSamples = 20;
    float diskRadius = 0.001 + pow(viewDistance, 1.1) * 0.001;

    for(int i = 0; i < pcfSamples; ++i) {
        vec3 sampleVec = lightVec + sampleOffsetDirections[i] * diskRadius;
        float sampleDistance = texture(texShadow, normalize(sampleVec)).x;
        if(length(lightVec) < sampleDistance + bias) shadow += 1.0;
    }

    return shadow / float(pcfSamples);
}

void main() {
    const vec3 normal = normalize(fragNormal);
    const mat3 TBN = mat3(fragTangent, fragBiTangent, fragNormal);

    vec2 texCoord = vec2(fragTexCoord.x, -fragTexCoord.y);

    float shading = useNormalMap ? lambertWithNormal(TBN, texCoord) : lambert(normal);
    shading += blinnPhong();
    float shadow = useShadow ? computeShadow() : 1.0;
    fragColor = shading * shadow * texture(texColor, texCoord);
    fragColor.w = 1.0;
}
