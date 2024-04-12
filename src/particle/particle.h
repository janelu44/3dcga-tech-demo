#pragma once
#include <glm/glm.hpp>

struct Particle {
    void reseed();

    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    float life{ -0.1f };
};
