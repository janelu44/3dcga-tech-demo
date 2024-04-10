#include "planet_system.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"

#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()
DISABLE_WARNINGS_POP()

PlanetSystem::PlanetSystem() {
    planets["Sun"] = &sun;
    sun.radius = 4.0f;
    sun.orbitRadius = 0.0f;
    sun.orbitSpeed = 0.0f;
    sun.revolutionSpeed = 0.0f;

    planets["Earth"] = &earth;
    earth.radius = 1.0f;
    earth.orbitRadius = sun.radius + 4.0f;
    earth.orbitAngle = glm::vec3(0.0f, 1.0f, 0.0f);
    earth.orbitSpeed = 0.2f;
    earth.baseColor = glm::vec3(0.0f, 0.5f, 1.0f);
    earth.loadColorMap("resources/textures/earth.jpg");
    earth.loadNormalMap("resources/normal/earth_normal.jpg");

    planets["Moon"] = &moon;
    moon.radius = 0.2f;
    moon.orbitRadius = earth.radius + 2.0f;
    moon.orbitAngle = glm::vec3(0.0f, 1.0f, 0.0f);
    moon.orbitSpeed = 2.0f;
    moon.baseColor = glm::vec3(0.9f, 0.9f, 0.9f);

    planets["Mars"] = &mars;
    mars.radius = 0.8f;
    mars.orbitRadius = earth.orbitRadius + 6.0f;
    mars.orbitAngle = glm::vec3(0.0f, 1.0f, 0.0f);
    mars.orbitSpeed = 0.4f;
    mars.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    mars.loadColorMap("resources/textures/mars.jpg");
    mars.loadNormalMap("resources/normal/mars_normal.jpg");
}

void PlanetSystem::drawGui() {
    float sameLineOffset = 100.0f;

    for (auto &planet: planets) {
        std::string planetName = planet.first;
        Planet *planetPtr = planet.second;
        if (ImGui::CollapsingHeader(planetName.c_str())) {
            ImGui::Text("Radius");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat(("##Radius" + planetName).c_str(), &planetPtr->radius, 0.01f);

            ImGui::Text("Orbit");
            ImGui::Separator();
            ImGui::Text("  Progress");
            ImGui::SameLine(sameLineOffset);
            ImGui::SliderFloat(("##OrbitProgress" + planetName).c_str(), &planetPtr->orbitProgress, 0.0f, 360.0f);
            ImGui::Text("  Radius");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat(("##OrbitRadius" + planetName).c_str(), &planetPtr->orbitRadius, 0.01f);
            ImGui::Text("  Speed");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat(("##OrbitSpeed" + planetName).c_str(), &planetPtr->orbitSpeed, 0.01f);
            ImGui::Text("  Angle");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat2(("##OrbitAngle" + planetName).c_str(), glm::value_ptr(planetPtr->orbitAngle), 0.1f);

            ImGui::Text("Revolution");
            ImGui::Separator();
            ImGui::Text("  Progress");
            ImGui::SameLine(sameLineOffset);
            ImGui::SliderFloat(("##RevolutionProgress" + planetName).c_str(), &planetPtr->revolutionProgress, 0.0f,
                               360.0f);
            ImGui::Text("  Speed");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat(("##RevolutionSpeed" + planetName).c_str(), &planetPtr->revolutionSpeed, 0.01f);
            ImGui::Text("  Angle");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat2(("##RevolutionAngle" + planetName).c_str(), glm::value_ptr(planetPtr->revolutionAngle),
                              0.1f);
        }
    }


}

void PlanetSystem::update() {
    sun.update(glm::mat4(1.0f));
    earth.update(sun.matRef);
    moon.update(earth.matRef);
    mars.update(sun.matRef);
}

void PlanetSystem::draw(glm::mat4 mvp, glm::vec3 cameraPos, ShadowMapFBO &shadowMap, bool useShadowMap,
                        const Shader &customShader, bool useCustomShader, bool renderSun) {

    for (auto &planet: planets) {
        if (!renderSun && planet.first == "Sun") {
            continue;
        }
        Planet *planetPtr = planet.second;
        if (useCustomShader) {
            customShader.bind();
        } else {
            planetPtr->bindShader();
        }
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(planetPtr->matScale));
        glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(planetPtr->getNormalMatrix()));
        glUniform3fv(5, 1, glm::value_ptr(cameraPos));
        glUniform3fv(6, 1, glm::value_ptr(lightPos));
        glUniform3fv(7, 1, glm::value_ptr(planetPtr->baseColor));
        glUniform1i(8, planet.first == "Sun" ? GL_TRUE : GL_FALSE);

        if (planetPtr->colorTexture.has_value()) {
            planetPtr->colorTexture.value().bind(GL_TEXTURE10);
            glUniform1i(30, 10);
        }
        if (planetPtr->normalTexture.has_value()) {
            planetPtr->normalTexture.value().bind(GL_TEXTURE11);
            glUniform1i(31, 11);
            glUniform1i(32, GL_TRUE);
        }
        glUniform1i(20, planet.first == "Sun" ? GL_FALSE : GL_TRUE);
        if (useShadowMap) {
            shadowMap.BindForReading(GL_TEXTURE21);
            glUniform1i(21, 21);
        }
        planetPtr->draw();
    }
}