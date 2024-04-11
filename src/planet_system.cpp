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
    sun.allowLightBehind = true;
    sun.bypassShadowMap = true;
//    sun.loadColorMap("resources/textures/sun.jpg");
    sun.loadDynamicTextures("resources/textures/dynamic/lsd/lsd_", 9, 1);


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

    planets["pbrTest"] = &pbrTest;
    pbrTest.radius = 0.8f;
    pbrTest.orbitRadius = mars.orbitRadius + 2.0f;
    pbrTest.orbitSpeed = 0.0f;
    pbrTest.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    pbrTest.revolutionSpeed = 0.02f;
    std::string pbr = "wornpaintedcement";
    std::string textureFolder = "resources/textures/pbr/" + pbr + "/";
    pbrTest.loadNormalMap(textureFolder + pbr + "_normal-ogl.png");
    pbrTest.loadAlbedoMap(textureFolder + pbr + "_albedo.png");
    pbrTest.loadMetallicMap(textureFolder + pbr + "_metallic.png");
    pbrTest.loadRoughnessMap(textureFolder + pbr + "_roughness.png");
    pbrTest.loadAoMap(textureFolder + pbr + "_ao.png");

    planets["pbrLight1"] = &pbrLight1;
    pbrLight1.radius = 0.05f;
    pbrLight1.orbitRadius = pbrTest.radius + 0.5f;
    pbrLight1.orbitSpeed = 0.2f;
    pbrLight1.baseColor = glm::vec3(1.0f, 0.0f, 0.0f);

    planets["pbrLight2"] = &pbrLight2;
    pbrLight2.radius = 0.05f;
    pbrLight2.orbitRadius = pbrTest.radius + 0.5f;
    pbrLight2.orbitAngle = glm::vec2(45.0f, 0.0f);
    pbrLight2.orbitSpeed = 0.3f;
    pbrLight2.orbitProgress = 60.0f;
    pbrLight2.baseColor = glm::vec3(1.0f, 1.0f, 0.0f);

    planets["pbrLight3"] = &pbrLight3;
    pbrLight3.radius = 0.05f;
    pbrLight3.orbitRadius = pbrTest.radius + 0.5f;
    pbrLight3.orbitAngle = glm::vec2(0.0f, 45.0f);
    pbrLight3.orbitSpeed = 0.4f;
    pbrLight3.orbitProgress = 150.0f;
    pbrLight3.baseColor = glm::vec3(0.0f, 1.0f, 0.0f);

    planets["pbrLight4"] = &pbrLight4;
    pbrLight4.radius = 0.05f;
    pbrLight4.orbitRadius = pbrTest.radius + 0.5f;
    pbrLight4.orbitAngle = glm::vec2(60.0f, 30.0f);
    pbrLight4.orbitSpeed = 0.5f;
    pbrLight4.orbitProgress = 230.0f;
    pbrLight4.baseColor = glm::vec3(0.0f, 1.0f, 1.0f);
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
            ImGui::DragFloat2(("##OrbitAngle" + planetName).c_str(), glm::value_ptr(planetPtr->orbitAngle), 0.1f, 0.0f,
                              360.0f);
            ImGui::Text("  Angle Shift");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat2(("##OrbitAngleShift" + planetName).c_str(), glm::value_ptr(planetPtr->orbitAngleShift),
                              0.01f);

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
                              0.1f, 0.0f, 360.0f);
            ImGui::Text("  Angle Shift");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat2(("##RevolutionAngleShift" + planetName).c_str(),
                              glm::value_ptr(planetPtr->revolutionAngleShift),
                              0.1f);
        }
    }


}

void PlanetSystem::update() {
    sun.update(glm::mat4(1.0f));
    earth.update(sun.matRef);
    moon.update(earth.matRef);
    mars.update(sun.matRef);
    pbrTest.update(sun.matRef);
    pbrLight1.update(pbrTest.matRef);
    pbrLight2.update(pbrTest.matRef);
    pbrLight3.update(pbrTest.matRef);
    pbrLight4.update(pbrTest.matRef);
}

void PlanetSystem::bindSpecificUniforms(std::string planetName) {
    if (planetName == "pbrTest") {
        glm::vec3 lightPositions[5] = {
                glm::vec3(0.0f),
                glm::vec3(pbrLight1.matPosition[3]),
                glm::vec3(pbrLight2.matPosition[3]),
                glm::vec3(pbrLight3.matPosition[3]),
                glm::vec3(pbrLight4.matPosition[3]),
        };
        glm::vec3 lightColors[5] = {
                glm::vec3(1.0f),
                glm::vec3(pbrLight1.baseColor),
                glm::vec3(pbrLight2.baseColor),
                glm::vec3(pbrLight3.baseColor),
                glm::vec3(pbrLight4.baseColor),
        };
        GLint prog = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

        GLint lightPositionsLocation = glGetUniformLocation(static_cast<GLuint>(prog), "lightPositions");
        glUniform3fv(lightPositionsLocation, 5, glm::value_ptr(lightPositions[0]));

        GLint lightColorsLocation = glGetUniformLocation(static_cast<GLuint>(prog), "lightColors");
        glUniform3fv(lightColorsLocation, 5, glm::value_ptr(lightColors[0]));
    }
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
        glUniform1i(8, planetPtr->allowLightBehind ? GL_TRUE : GL_FALSE);
        if (!useCustomShader) {
            if (planetPtr->dynamicTextures.has_value()) {
                planetPtr->dynamicTextures.value()[dynamicTextureIteration / dynamicTextureFrametime].bind(
                        GL_TEXTURE10);
                glUniform1i(30, 10);
                dynamicTextureIteration++;
                dynamicTextureIteration %= (planetPtr->dynamicTextures.value().size() * dynamicTextureFrametime);

            }
            if (planetPtr->colorTexture.has_value()) {
                planetPtr->colorTexture.value().bind(GL_TEXTURE10);
                glUniform1i(30, 10);
            }
            if (planetPtr->normalTexture.has_value()) {
                planetPtr->normalTexture.value().bind(GL_TEXTURE11);
                glUniform1i(31, 11);
                glUniform1i(32, GL_TRUE);
            }
            if (planetPtr->albedoTexture.has_value()) {
                planetPtr->albedoTexture.value().bind(GL_TEXTURE12);
                glUniform1i(33, 12);
                glUniform1i(34, GL_TRUE);
            }
            if (planetPtr->metallicTexture.has_value()) {
                planetPtr->metallicTexture.value().bind(GL_TEXTURE13);
                glUniform1i(35, 13);
                glUniform1i(36, GL_TRUE);
            }
            if (planetPtr->roughnessTexture.has_value()) {
                planetPtr->roughnessTexture.value().bind(GL_TEXTURE14);
                glUniform1i(37, 14);
                glUniform1i(38, GL_TRUE);
            }
            if (planetPtr->aoTexture.has_value()) {
                planetPtr->aoTexture.value().bind(GL_TEXTURE15);
                glUniform1i(39, 15);
                glUniform1i(40, GL_TRUE);
            }
        }

        glUniform1i(20, planetPtr->bypassShadowMap ? GL_TRUE : GL_FALSE);
        if (useShadowMap) {
            shadowMap.BindForReading(GL_TEXTURE21);
            glUniform1i(21, 21);
        }
        if (!useCustomShader) {
            bindSpecificUniforms(planet.first);
        }
        planetPtr->draw();
    }
}