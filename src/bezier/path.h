#pragma once

#include "curve.h"
#include <vector>

class BezierPath {
public:
	BezierPath(BezierCurve startingSection);
	BezierPath(BezierCurve startingSection, std::vector<std::pair<glm::vec3, glm::vec3>> nextSections, bool closed);

	void addSection(glm::vec3 endpoint, glm::vec3 control);
	void addSection(glm::vec3 startpointControl, glm::vec3 endpoint, glm::vec3 endpointControl);
	void makeClosed();

	glm::vec3 evaluate(float t);

public:
	std::vector<BezierCurve> curves;
};
