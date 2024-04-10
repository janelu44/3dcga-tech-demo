#include "curve.h"
#include <cmath>
#include <glm/glm.hpp>

BezierCurve::BezierCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) : points{ a, b, c, d } {
	// precomputing components of polynomial's derivative
	vt2 = -3.0f * a + 9.0f * b - 9.0f * c + 3.0f * d; // * t^2
	vt = 6.0f * a - 12.0f * b + 6.0f * c; // * t
	v = -3.0f * a + 3.0f * b;// * 1
}

glm::vec3 BezierCurve::evaluate(float t) {
	glm::vec3 b{ 0.0f };
	for (int i = 3; i >= 0; i--) {
		float factor = pow(t, i) * pow((1 - t), 3 - i) * ((i == 1 || i == 2) ? 3 : 1);
		b += points[i] * factor;
	}
	return b;
}

float BezierCurve::advanceDirectly(float t, float& l) {
	float dt = glm::length(t * t * vt2 + t * vt + v);
	double tDelta = l / dt;

	if (t + tDelta >= 1.0f) {
		double tDiff = t + tDelta - 1.0f;
		t = 1.0f;
		l = tDiff * dt;
	}
	else {
		t += tDelta;
		l = 0.0f;
	}
	return t;
}

float BezierCurve::advance(float t, float& l) {
	constexpr int ITERS = 10;

	for (int i = 0; i < ITERS; i++) {
		float partL = l / ITERS;
		t = advanceDirectly(t, partL);
		l -= l / ITERS - partL;
		if (partL != 0.0f)
			return t; // no reason to continue now
	}

	l = 0.0f;
	return t;
}
