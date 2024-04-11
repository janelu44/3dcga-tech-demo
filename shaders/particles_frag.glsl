#version 450

layout(location = 5) uniform vec3 endColor;

layout(location = 0) in vec3 position;
layout(location = 1) in float life;

layout(location = 0) out vec4 fragColor;

void main() {
	vec3 color = mix(endColor, vec3(1), life / 100);
	fragColor = vec4(color, 1);
};
