#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()

#include <glm/mat4x4.hpp>
#include <map>

DISABLE_WARNINGS_POP()

#include "texture.h"
#include "planet.h"
#include "shadow/shadow_map_fbo.h"
#include "environment/env_map.h"

class PlanetSystem {
    std::map<std::string, Planet *> planets;
    glm::vec3 lightPos = glm::vec3(0.0f);
    int dynamicTextureIteration = 0;
    int dynamicTextureFrametime = 5;

    Shader defaultShader = ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl")
            .build();
    Shader materialShader = ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, "shaders/material_frag.glsl")
            .build();
    Shader colorShader = ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, "shaders/minimapColor_frag.glsl")
            .build();
    Shader textureShader = ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, "shaders/texture_frag.glsl")
            .build();
    Shader pbrShader = ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, "shaders/pbr_frag.glsl")
            .build();
    Shader reflectionShader = ShaderBuilder()
            .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, "shaders/environment_frag.glsl")
            .build();

    std::string sphereMeshPath = "resources/meshes/rhino_sphere.obj";

    // SOLAR SYSTEM
    Planet sun = Planet(sphereMeshPath, textureShader);
    Planet earth = Planet(sphereMeshPath, textureShader);
    Planet moon = Planet(sphereMeshPath, defaultShader);
    Planet mars = Planet(sphereMeshPath, textureShader);
    Planet dynamic = Planet(sphereMeshPath, textureShader);

    // PBR
    Planet pbr = Planet(sphereMeshPath, pbrShader);
    Planet pbrLight1 = Planet(sphereMeshPath, colorShader);
    Planet pbrLight2 = Planet(sphereMeshPath, colorShader);
    Planet pbrLight3 = Planet(sphereMeshPath, colorShader);
    Planet pbrLight4 = Planet(sphereMeshPath, colorShader);

    // ENV
    Planet env = Planet(sphereMeshPath, reflectionShader);
    Planet envChild1 = Planet(sphereMeshPath, colorShader);
    Planet envChild2 = Planet(sphereMeshPath, colorShader);
    Planet envChild3 = Planet(sphereMeshPath, colorShader);
    Planet envChild4 = Planet(sphereMeshPath, colorShader);
    Planet envChild5 = Planet(sphereMeshPath, colorShader);
    Planet envChild6 = Planet(sphereMeshPath, colorShader);

public:
    PlanetSystem();

    void drawGui();

    void update();

    void bindSpecificUniforms(std::string planetName);

    void
    draw(glm::mat4 mvp, glm::vec3 cameraPos, ShadowMapFBO &shadowMap, bool useShadowMap, const Shader &customShader,
         bool useCustomShader, EnvMap envMap, bool useEnvMap);

    glm::vec3 getEnvMapPosition();
};