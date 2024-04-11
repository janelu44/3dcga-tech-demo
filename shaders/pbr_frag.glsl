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


// Shader-specific uniforms
layout(location = 50) uniform vec3 uAlbedo = vec3(0.2f);
layout(location = 51) uniform float uMetallic = 0.0f;
layout(location = 52) uniform float uRoughness = 0.3f;
layout(location = 53) uniform float uAo = 1.0f;

uniform vec3 lightPositions[5];
uniform vec3 lightColors[5];

const float PI = 3.14159265359;

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

vec3 normalFromMap(mat3 tbn) {
    vec3 normal = texture(texNormal, fragTexCoord).rgb;
    normal = normal * 2.0 - 1.0;
    return normalize(tbn * normal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    const mat3 TBN = mat3(fragTangent, fragBiTangent, fragNormal);

    vec3 albedo = useAlbedoMap ? pow(texture(texAlbedo, fragTexCoord).rgb, vec3(2.2)) : uAlbedo;
    vec3 normal = useNormalMap ? normalFromMap(TBN) : fragNormal;
    float metallic = useMetallicMap ? texture(texMetallic, fragTexCoord).r : uMetallic;
    float roughness = useRoughnessMap ? texture(texRoughness, fragTexCoord).r : uRoughness;
    float ao = useAoMap ? texture(texAo, fragTexCoord).r : uAo;

    vec3 N = normalize(normal);
    vec3 V = normalize(viewPos - fragPos);

    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metallic);

    // Reflectance
    vec3 Lo = vec3(0.0f);
    for (int i = 0; i < 5; i++) {
        // Per-light radiance
        vec3 L = normalize(lightPositions[i] - fragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= 1.0f - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f;
        vec3 specular = numerator / denominator;

        // Add to total radiance
        float NdotL = max(dot(N, L), 0.0f);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0f));
    color = pow(color, vec3(1.0f/2.2f));

    fragColor = vec4(color, 1.0f);
}