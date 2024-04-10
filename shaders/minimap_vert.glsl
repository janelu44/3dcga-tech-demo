#version 450

layout(location = 0) uniform mat4 mvpMatrix;
layout(location = 1) uniform mat4 modelMatrix;

layout(location = 0) in vec3 position;

out vec3 fragPos;
out vec2 fragTexCoord;

void main()
{
    fragPos = (modelMatrix * vec4(position, 1)).xyz;
    fragTexCoord = (position.xy + 1.0) * 0.5;

    gl_Position = mvpMatrix * vec4(fragPos, 1);
}
