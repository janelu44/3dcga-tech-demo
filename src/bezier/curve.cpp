#include "curve.h"

BezierCurve::BezierCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) : points{ a, b, c, d } {
}

glm::vec3 BezierCurve::evaluate(float t) {
	glm::vec3 b{ 0.0f };
	float factor = t * t * t;

	for (int i = 3; i >= 0; i--) {
		b += points[i] * factor;
		factor = factor * (1 - t) / t;
	}
	return b;
}
