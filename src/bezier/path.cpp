#include "path.h"
#include <iostream>

BezierPath::BezierPath(BezierCurve startingSection) {
	curves.push_back(startingSection);
}

BezierPath::BezierPath(BezierCurve startingSection, std::vector<std::pair<glm::vec3, glm::vec3>> nextSections, bool closed) {
	curves.push_back(startingSection);
	for (const auto& [point, control] : nextSections)
		addSection(point, control);
	if (closed)
		makeClosed();
}


void BezierPath::addSection(glm::vec3 endpoint, glm::vec3 control) {
	glm::vec3 startpointControl = 2.0f * curves.back().points[3] - curves.back().points[2];
	addSection(startpointControl, endpoint, control);
}

void BezierPath::addSection(glm::vec3 startpointControl, glm::vec3 endpoint, glm::vec3 endpointControl) {
	curves.push_back(BezierCurve{ curves.back().points[3], startpointControl, endpointControl, endpoint });
}

void BezierPath::makeClosed() {
	glm::vec3 endpoint = curves.front().points[0];
	glm::vec3 control = 2.0f * endpoint - curves.front().points[1];
	addSection(endpoint, control);
}

glm::vec3 BezierPath::evaluate(float t) {
	float normalizedT = curves.size() * t;

	int currentCurve = int(normalizedT);
	if (currentCurve == curves.size())
		currentCurve -= 1;
	float currentT = normalizedT - currentCurve;

	return curves[currentCurve].evaluate(currentT);
}
