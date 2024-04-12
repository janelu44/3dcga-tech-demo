#include "particle.h"

#include <stdlib.h>
#include <glm/ext/scalar_constants.hpp>

void Particle::reseed() {
	const auto& random = [](float a = 0.0f, float b = 1.0f) {
		float s = (float)rand() / RAND_MAX;
		return a + (b - a) * s;
		};

	float r = 30.0f * sqrt(random());
	float theta = 2.0f * glm::pi<float>() * random();

	position.x = r * sin(theta);
	position.y = r * cos(theta);
	position.z = random(0.0f, 16.0f);

	velocity.x = 0.0f;
	velocity.y = 0.0f;
	velocity.z = random(0.01f, 0.1f);

	life = random(100.0f, 1000.0f);
}
