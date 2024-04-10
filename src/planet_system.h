#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()

#include <glm/mat4x4.hpp>
#include <map>

DISABLE_WARNINGS_POP()

#include "texture.h"
#include "planet.h"
#include "shadow/shadow_map_fbo.h"

class PlanetSystem {
    std::map<std::string, Planet *> planets;
    glm::vec3 lightPos = glm::vec3(0.0f);

    Shader colorShader = ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl")
            .build();
    Shader textureShader = ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, "shaders/texture_frag.glsl")
            .build();

    Planet sun = Planet("resources/meshes/sphere.obj", colorShader);
    Planet earth = Planet("resources/meshes/sphere.obj", textureShader);
    Planet moon = Planet("resources/meshes/sphere.obj", colorShader);
    Planet mars = Planet("resources/meshes/sphere.obj", textureShader);

public:
    PlanetSystem();

    void drawGui();

    void update();

    void
    draw(glm::mat4 mvp, glm::vec3 cameraPos, ShadowMapFBO &shadowMap, bool useShadowMap, const Shader &customShader,
         bool useCustomShader, bool renderSun);
};