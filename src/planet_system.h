#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
DISABLE_WARNINGS_POP()

#include "planet.h"

class PlanetSystem {
public:
    PlanetSystem();

    void update();
    void draw(glm::mat4 mvp);
    void drawShadowMap(const Shader& shadowShader, glm::mat4 mvp);
};