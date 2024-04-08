#version 450

layout(location = 3) uniform sampler2D texColor;
layout(location = 4) uniform bool hasTexCoords;
layout(location = 5) uniform vec3 viewPos = vec3(0.0);
layout(location = 6) uniform vec3 lightPos;
layout(location = 7) uniform vec3 forceColor = vec3(1.0);
layout(location = 8) uniform bool ignoreBehind = false;
layout(location = 9) uniform sampler2D texNormal;

in vec3 fragPos;
in vec3 fragNormal;
in vec3 fragTangent;
in vec3 fragBiTangent;
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

float lambertWithBump(mat3 tbn) {
    vec3 normal = texture(texNormal, fragTexCoord).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(tbn * normal);
    float l = dot(normal, normalize(lightPos - fragPos));
    return max(l, 0.0);
}


float blinnPhong(bool ignoreBehind) {
    vec3 H = normalize(viewPos - fragPos + lightPos - fragPos);
    vec3 N = normalize(fragNormal);
    float d = dot(H, N);
    if (ignoreBehind) d = abs(d);
    if (dot(lightPos - fragPos, fragNormal) <= 0.0) {
        d = 0.0;
    }
    return pow(d, fragShininess);
}

void main()
{
    const vec3 normal = normalize(fragNormal);

    mat3 TBN = mat3(fragTangent, fragBiTangent, fragNormal);

    if (hasTexCoords) {
//        fragColor = texture(texColor, fragTexCoord);
        fragColor = vec4(lambertWithBump(TBN) * texture(texColor, fragTexCoord).rgb, 1);
        fragColor = vec4(fragTexCoord.x, fragTexCoord.y, 0, 1);
    } else {
        fragColor = vec4(lambert(ignoreBehind) * forceColor + forceColor * 0.1f, 1);
    }
}
