#include "planet_system.h"
#include "glm/gtc/type_ptr.hpp"
#include "texture.h"

#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()
DISABLE_WARNINGS_POP()

glm::vec3 lightPos = glm::vec3(0.0f);

Planet sun = Planet("resources/meshes/sphere.obj");
Shader sunShader = ShaderBuilder()
        .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
        .addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl")
        .build();

Planet earth = Planet("resources/meshes/sphere.obj");
Texture earthTexture = Texture("resources/textures/earth.jpg");
Texture earthNormal = Texture("resources/normal/earth_normal.jpg");
Shader earthShader = ShaderBuilder()
        .addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl")
        .addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl")
        .build();

PlanetSystem::PlanetSystem() {
    sun.radius = 4.0f;
    sun.orbitRadius = 0.0f;
    sun.orbitSpeed = 0.0f;
    sun.revolutionSpeed = 0.0f;

    earth.radius = 1.0f;
    earth.orbitRadius = sun.radius + 2.0f;
    earth.orbitAngle = glm::vec3(0.0f, 1.0f, 0.0f);
    earth.orbitSpeed = 0.2f;
}

void PlanetSystem::update() {
    sun.update(glm::mat4(1.0f));
    earth.update(sun.matPosition);
}

void PlanetSystem::draw(glm::mat4 mvp) {
    // DRAW SUN
    sunShader.bind();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(sun.matScale));
    glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(sun.getNormalMatrix()));
    glUniform1i(4, GL_FALSE);
    glUniform3fv(6, 1, glm::value_ptr(lightPos));
    glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.0f, 0.5f, 1.0f)));
    sun.draw();

    // DRAW EARTH
    earthShader.bind();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(sun.matScale));
    glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(sun.getNormalMatrix()));
    earthTexture.bind(GL_TEXTURE0);
    glUniform1i(3, 0);
    glUniform1i(4, GL_TRUE);
    glUniform3fv(6, 1, glm::value_ptr(lightPos));
    glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.0f, 0.5f, 1.0f)));
    earthNormal.bind(GL_TEXTURE1);
    glUniform1i(9, 1);
    earth.draw();
}

void PlanetSystem::drawShadowMap(const Shader &shadowShader, glm::mat4 mvp) {
    // DRAW EARTH
    shadowShader.bind();
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(sun.matScale));
    glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(sun.getNormalMatrix()));
    earthTexture.bind(GL_TEXTURE0);
    glUniform1i(3, 0);
    glUniform1i(4, GL_TRUE);
    glUniform3fv(6, 1, glm::value_ptr(lightPos));
    glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.0f, 0.5f, 1.0f)));
    earthNormal.bind(GL_TEXTURE1);
    glUniform1i(9, 1);
    earth.draw();
}