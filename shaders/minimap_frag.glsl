#version 450

layout(location = 2) uniform sampler2D texMinimap;

in vec3 fragPos;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

void main() {
    vec2 texCoord = fragTexCoord * textureSize(texMinimap, 0);
    fragColor = texture(texMinimap, texCoord);
}
