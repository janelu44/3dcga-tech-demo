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
    int dynamicTextureIteration = 0;
    int dynamicTextureFrametime = 5;

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
    
    std::string sphereMeshPath = "resources/meshes/rhino_sphere.obj";

    Planet sun = Planet(sphereMeshPath, textureShader);
    Planet earth = Planet(sphereMeshPath, textureShader);
    Planet moon = Planet(sphereMeshPath, colorShader);
    Planet mars = Planet(sphereMeshPath, textureShader);
    Planet pbrTest = Planet(sphereMeshPath, pbrShader);
    Planet pbrLight1 = Planet(sphereMeshPath, colorShader);
    Planet pbrLight2 = Planet(sphereMeshPath, colorShader);
    Planet pbrLight3 = Planet(sphereMeshPath, colorShader);
    Planet pbrLight4 = Planet(sphereMeshPath, colorShader);

public:
    PlanetSystem();

    void drawGui();

    void update();

    void bindSpecificUniforms(std::string planetName);

    void
    draw(glm::mat4 mvp, glm::vec3 cameraPos, ShadowMapFBO &shadowMap, bool useShadowMap, const Shader &customShader,
         bool useCustomShader, bool renderSun);

};