#version 450

layout(location = 2) uniform sampler2D texMinimap;

in vec3 fragPos;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

void main() {
    float xRes = textureSize(texMinimap, 0).x;
    float yRes = textureSize(texMinimap, 0).y;

    float x = (fragTexCoord.x + xRes/2000.0) / (xRes/1000.0);
    float y = (fragTexCoord.y + yRes/2000.0) / (yRes/1000.0);

    fragColor = texture(texMinimap, vec2(x, y));
}
