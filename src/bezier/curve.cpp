#include "curve.h"
#include <cmath>

BezierCurve::BezierCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) : points{ a, b, c, d } {
}

glm::vec3 BezierCurve::evaluate(float t) {
	glm::vec3 b{ 0.0f };
	for (int i = 3; i >= 0; i--) {
		float factor = pow(t, i) * pow((1 - t), 3 - i) * ((i == 1 || i == 2) ? 3 : 1);
		b += points[i] * factor;
	}
	return b;
}
