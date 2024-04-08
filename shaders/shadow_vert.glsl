#version 450

layout(location = 0) uniform mat4 mvpMatrix;
layout(location = 1) uniform mat4 modelMatrix;
layout(location = 2) uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 6) in vec2 texCoord;

out vec3 fragPos;

void main()
{
    vec4 pos4 = vec4(position, 1.0);

    fragPos = (modelMatrix * pos4).xyz;
    gl_Position = mvpMatrix * pos4;
}
