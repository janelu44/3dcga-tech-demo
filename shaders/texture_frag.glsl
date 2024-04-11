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

vec3 currentLightPos = isNight && useSpotlight ? spotlightPos : lightPos;

float lambert(vec3 normal) {
    float l = dot(normal, normalize(currentLightPos - fragPos));
    return max(l, 0.0);
}

float lambertWithNormal(mat3 tbn, vec2 texCoord) {
    vec3 normal = texture(texNormal, texCoord).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(tbn * normal);
    return lambert(normal);
}

float blinnPhong() {
    vec3 H = normalize(viewPos - fragPos + currentLightPos - fragPos);
    vec3 N = normalize(fragNormal);
    float d = dot(H, N);
    if (dot(currentLightPos - fragPos, fragNormal) <= 0.0) {
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
    const vec3 lightVec = fragPos - currentLightPos;
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

float computeSpotlight() {
    const vec3 lightDir = normalize(fragPos - spotlightPos);

    vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);
    fragLightCoord.xyz /= fragLightCoord.w;
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;

    float shadow = 0.0;
    float bias = 0.005 * (1.0 - dot(fragNormal, lightDir));
    int pcfRadius = 3;
    for (int i = -pcfRadius; i <= pcfRadius; ++i) {
        for (int j = -pcfRadius; j <= pcfRadius; ++j) {
            float sampleDistance = texture(texSpotlight, fragLightCoord.xy + vec2(i, j)/textureSize(texSpotlight, 0).x).x;
            if (length(fragPos - spotlightPos) < sampleDistance + bias) shadow += 1.0;
        }
    }
    shadow /= pow(1 + pcfRadius, 2);

    float distToCenter = distance(fragLightCoord.xy, vec2(0.5, 0.5));
    float spotLightMultiplier = distToCenter >= 0.5 ? 0.0 : 1 - distToCenter * 2.0;
    return shadow * spotLightMultiplier;
}

void main() {
    const vec3 normal = normalize(fragNormal);
    const mat3 TBN = mat3(fragTangent, fragBiTangent, fragNormal);

    vec2 texCoord = vec2(fragTexCoord.x, -fragTexCoord.y);

    float shading = useNormalMap ? lambertWithNormal(TBN, texCoord) : lambert(normal);
    shading += blinnPhong();
    vec3 color = shading * texture(texColor, texCoord).rgb;

    float shadow = useShadow ? computeShadow() : 1.0;
    float spotlight = useSpotlight ? computeSpotlight() : 1.0;

    fragColor = vec4(color * shadow, 1.0);
    if (isNight) fragColor = vec4(color * (useSpotlight ? max(spotlight, 0.1f) : 0.1f) , 1.0f);
}
