#pragma once

#include <glm/vec3.hpp>

class BezierCurve {
public:
	BezierCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
	glm::vec3 evaluate(float t);

public:
	glm::vec3 points[4];
};
