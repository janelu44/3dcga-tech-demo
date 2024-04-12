#version 450

layout(location = 0) uniform mat4 mvpMatrix;
layout(location = 1) uniform mat4 modelMatrix;
layout(location = 2) uniform vec3 cameraUp;
layout(location = 3) uniform vec3 cameraRight;
layout(location = 4) uniform vec2 billboardSize;

layout(location = 0) in vec3 billboardPosition;
layout(location = 1) in vec4 particleData;

layout(location = 0) out vec3 position;
layout(location = 1) out float life;

void main() {
	vec3 center = particleData.xyz;
	vec3 worldCenter = (modelMatrix * vec4(center, 1)).xyz;
	vec2 size = billboardSize / sqrt(center.z / 10);
	vec3 worldPosition = worldCenter
		+ cameraUp * billboardPosition.x * size.x
		+ cameraRight * billboardPosition.y * size.y;
	gl_Position = mvpMatrix * vec4(worldPosition, 1);

	position = center;
	life = particleData.w;
}
