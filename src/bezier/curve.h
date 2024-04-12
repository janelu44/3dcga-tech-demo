#pragma once

#include <glm/vec3.hpp>

class BezierCurve {
public:
	BezierCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
	glm::vec3 evaluate(float t);
	float advance(float t, float& l);
	float advanceDirectly(float t, float& l);

public:
	glm::vec3 points[4];
	glm::vec3 vt2, vt, v;
};
