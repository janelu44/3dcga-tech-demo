#version 450 core

#define PI 3.1415926538
#define EPS 1e-6f

in vec2 coord;

layout(location = 0) out vec4 outColor;

layout(location = 0) uniform float Kernel_Gaussian_Magnitude;
layout(location = 1) uniform float Kernel_Gaussian_Bandwidth;
layout(location = 2) uniform float Kernel_Harmonic_Frequency_Magnitude;
layout(location = 3) uniform float Kernel_Harmonic_Frequency_Magnitude_Spread;
layout(location = 4) uniform float Kernel_Harmonic_Frequency_Orientation;
layout(location = 5) uniform float Kernel_Harmonic_Frequency_Orientation_Spread;

layout(location = 6) uniform float Scale;

// aproximare jalnica da aia e
// mai are buguri cand ies ang1 si ang2 din intervalul -pi, pi
bool test(vec2 pos) {
	float mag = sqrt(dot(pos, pos));
	float ang_delta = 2.0 * asin(Kernel_Gaussian_Bandwidth / 2.0 / mag);

	
		if (distance(pos,
		            vec2(-Kernel_Harmonic_Frequency_Magnitude * sin(Kernel_Harmonic_Frequency_Orientation),
					     Kernel_Harmonic_Frequency_Magnitude * cos(Kernel_Harmonic_Frequency_Orientation)))
		   < Kernel_Gaussian_Bandwidth / 2.0)
			return true;
	


	float ang = atan(pos.y, -pos.x);
	if (abs(mag - Kernel_Harmonic_Frequency_Magnitude) < Kernel_Harmonic_Frequency_Magnitude_Spread / 2.0 + Kernel_Gaussian_Bandwidth / 2.0 + EPS) {
		float ang1 = Kernel_Harmonic_Frequency_Orientation - Kernel_Harmonic_Frequency_Orientation_Spread / 2.0;
		float ang2 = Kernel_Harmonic_Frequency_Orientation + Kernel_Harmonic_Frequency_Orientation_Spread / 2.0;
		if (ang1 <= ang && ang <= ang2)
			return true;
	}
	return false;
}

void main() {
	vec2 sCoord = coord * Scale;
	outColor = vec4(float(test(sCoord) || test(-sCoord)));
}
