#include "planet_system.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui/imgui.h"
#include "camera.h"
#include "shadow/shadow_directions.h"

#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()
DISABLE_WARNINGS_POP()

PlanetSystem::PlanetSystem() {
    planets["Sun"] = &sun;
    sun.radius = 4.0f;
    sun.orbitRadius = 0.0f;
    sun.orbitSpeed = 0.0f;
    sun.revolutionSpeed = 0.1f;
    sun.allowLightBehind = true;
    sun.bypassShadowMap = true;
    sun.loadColorMap("resources/textures/sun.jpg");

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
    mars.baseColor = glm::vec3(0.624f, 0.424f, 0.302f);
    mars.loadColorMap("resources/textures/mars.jpg");
    mars.loadNormalMap("resources/normal/mars_normal.jpg");

    planets["Dynamic"] = &dynamic;
    dynamic.radius = 1.4f;
    dynamic.orbitRadius = mars.orbitRadius;
    dynamic.orbitAngle = glm::vec2(20.0f, 0.0f);
    dynamic.orbitProgress = 160.0f;
    dynamic.orbitSpeed = 0.4f;
    dynamic.revolutionSpeed = 0.3f;
    dynamic.baseColor = glm::vec3(0.98f, 0.784f, 0.784f);
    dynamic.loadDynamicTextures("resources/textures/dynamic/trip/trip_", 30, 2);

    planets["pbr"] = &pbr;
    pbr.radius = 0.8f;
    pbr.orbitRadius = mars.orbitRadius + 2.0f;
    pbr.orbitSpeed = 0.1f;
    pbr.orbitAngle = glm::vec2(0.0f, -10.0f);
    pbr.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    pbr.revolutionSpeed = 0.02f;
    std::string pbrMaterial = "alien-carniverous-plant";
    std::string textureFolder = "resources/textures/pbr/" + pbrMaterial + "/";
    pbr.loadNormalMap(textureFolder + pbrMaterial + "_normal-ogl.png");
    pbr.loadAlbedoMap(textureFolder + pbrMaterial + "_albedo.png");
    pbr.loadMetallicMap(textureFolder + pbrMaterial + "_metallic.png");
    pbr.loadRoughnessMap(textureFolder + pbrMaterial + "_roughness.png");
    pbr.loadAoMap(textureFolder + pbrMaterial + "_ao.png");

    planets["pbrLight1"] = &pbrLight1;
    pbrLight1.radius = 0.05f;
    pbrLight1.orbitRadius = pbr.radius + 0.5f;
    pbrLight1.orbitAngleShift = glm::vec2(0.2f, 0.15f);
    pbrLight1.orbitSpeed = -0.2f;
    pbrLight1.baseColor = glm::vec3(1.0f, 0.0f, 0.0f);

    planets["pbrLight2"] = &pbrLight2;
    pbrLight2.radius = 0.05f;
    pbrLight2.orbitRadius = pbr.radius + 0.5f;
    pbrLight2.orbitAngle = glm::vec2(45.0f, 0.0f);
    pbrLight1.orbitAngleShift = glm::vec2(-0.2f, 0.4f);
    pbrLight2.orbitSpeed = 0.3f;
    pbrLight2.orbitProgress = 60.0f;
    pbrLight2.baseColor = glm::vec3(1.0f, 1.0f, 0.0f);

    planets["pbrLight3"] = &pbrLight3;
    pbrLight3.radius = 0.05f;
    pbrLight3.orbitRadius = pbr.radius + 0.5f;
    pbrLight3.orbitAngle = glm::vec2(0.0f, 45.0f);
    pbrLight1.orbitAngleShift = glm::vec2(0.2f, -0.15f);
    pbrLight3.orbitSpeed = -0.4f;
    pbrLight3.orbitProgress = 150.0f;
    pbrLight3.baseColor = glm::vec3(0.0f, 1.0f, 0.0f);

    planets["pbrLight4"] = &pbrLight4;
    pbrLight4.radius = 0.05f;
    pbrLight4.orbitRadius = pbr.radius + 0.5f;
    pbrLight4.orbitAngle = glm::vec2(60.0f, 30.0f);
    pbrLight1.orbitAngleShift = glm::vec2(-0.1f, -0.1f);
    pbrLight4.orbitSpeed = 0.5f;
    pbrLight4.orbitProgress = 230.0f;
    pbrLight4.baseColor = glm::vec3(0.0f, 1.0f, 1.0f);

    planets["env"] = &env;
    env.radius = 1.0f;
    env.orbitRadius = pbr.orbitRadius;
    env.orbitProgress = 180.0f;
    env.orbitSpeed = 0.2f;
    env.revolutionSpeed = 0.0f;
    env.baseColor = glm::vec3(1.0f, 0.0f, 1.0f);
    env.bypassEnvMap = true;

    float envChildSize = 0.2f;

    planets["envChild1"] = &envChild1;
    envChild1.radius = envChildSize;
    envChild1.orbitRadius = env.radius + 0.5f;
    envChild1.orbitSpeed = 0.2f;
    envChild1.baseColor = glm::vec3(1.0f, 0.0f, 0.0f);

    planets["envChild2"] = &envChild2;
    envChild2.radius = envChildSize;
    envChild2.orbitRadius = env.radius + 0.5f;
    envChild2.orbitAngle = glm::vec2(45.0f, 0.0f);
    envChild2.orbitSpeed = 0.3f;
    envChild2.orbitProgress = 60.0f;
    envChild2.baseColor = glm::vec3(1.0f, 1.0f, 0.0f);

    planets["envChild3"] = &envChild3;
    envChild3.radius = envChildSize;
    envChild3.orbitRadius = env.radius + 0.5f;
    envChild3.orbitAngle = glm::vec2(0.0f, 45.0f);
    envChild3.orbitSpeed = 0.4f;
    envChild3.orbitProgress = 150.0f;
    envChild3.baseColor = glm::vec3(0.0f, 1.0f, 0.0f);

    planets["envChild4"] = &envChild4;
    envChild4.radius = envChildSize;
    envChild4.orbitRadius = env.radius + 0.5f;
    envChild4.orbitAngle = glm::vec2(60.0f, 30.0f);
    envChild4.orbitSpeed = 0.5f;
    envChild4.orbitProgress = 230.0f;
    envChild4.baseColor = glm::vec3(0.0f, 1.0f, 1.0f);

    planets["envChild5"] = &envChild5;
    envChild5.radius = envChildSize;
    envChild5.orbitRadius = env.radius + 0.5f;
    envChild5.orbitAngle = glm::vec2(-30.0f, 30.0f);
    envChild5.orbitSpeed = 0.7f;
    envChild5.orbitProgress = 20.0f;
    envChild5.baseColor = glm::vec3(1.0f, 0.0f, 1.0f);

    planets["envChild6"] = &envChild6;
    envChild6.radius = envChildSize;
    envChild6.orbitRadius = env.radius + 0.5f;
    envChild6.orbitAngle = glm::vec2(80.0f, -10.0f);
    envChild6.orbitSpeed = 0.1f;
    envChild6.orbitProgress = 300.0f;
    envChild6.baseColor = glm::vec3(1.0f, 0.5f, 0.0f);
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
    dynamic.update(sun.matRef);

    pbr.update(sun.matRef);
    pbrLight1.update(pbr.matRef);
    pbrLight2.update(pbr.matRef);
    pbrLight3.update(pbr.matRef);
    pbrLight4.update(pbr.matRef);
    
    env.update(sun.matRef);
    envChild1.update(env.matRef);
    envChild2.update(env.matRef);
    envChild3.update(env.matRef);
    envChild4.update(env.matRef);
    envChild5.update(env.matRef);
    envChild6.update(env.matRef);
}

void PlanetSystem::bindSpecificUniforms(std::string planetName) {
    if (planetName == "pbr") {
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
                        const Shader &customShader, bool useCustomShader, EnvMap envMap, bool useEnvMap) {

    for (auto &planet: planets) {
        Planet *planetPtr = planet.second;
        if (!useShadowMap && planetPtr->bypassShadowMap) {
            continue;
        }
        if (!useEnvMap && planetPtr->bypassEnvMap) {
            continue;
        }
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

        glUniform1i(20, useShadowMap ? GL_TRUE : GL_FALSE);
        if (useShadowMap) {
            shadowMap.BindForReading(GL_TEXTURE21);
            glUniform1i(21, 21);
        }
        if (!useCustomShader) {
            bindSpecificUniforms(planet.first);
        }

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
            } else {
                glUniform1i(32, GL_FALSE);
            }
            if (planetPtr->albedoTexture.has_value()) {
                planetPtr->albedoTexture.value().bind(GL_TEXTURE12);
                glUniform1i(33, 12);
                glUniform1i(34, GL_TRUE);
            } else {
                glUniform1i(34, GL_FALSE);
            }
            if (planetPtr->metallicTexture.has_value()) {
                planetPtr->metallicTexture.value().bind(GL_TEXTURE13);
                glUniform1i(35, 13);
                glUniform1i(36, GL_TRUE);
            } else {
                glUniform1i(36, GL_FALSE);
            }
            if (planetPtr->roughnessTexture.has_value()) {
                planetPtr->roughnessTexture.value().bind(GL_TEXTURE14);
                glUniform1i(37, 14);
                glUniform1i(38, GL_TRUE);
            } else {
                glUniform1i(38, GL_FALSE);
            }
            if (planetPtr->aoTexture.has_value()) {
                planetPtr->aoTexture.value().bind(GL_TEXTURE15);
                glUniform1i(39, 15);
                glUniform1i(40, GL_TRUE);
            } else {
                glUniform1i(40, GL_FALSE);
            }

            if (useEnvMap) {
                envMap.BindForReading(GL_TEXTURE29);
                glUniform1i(60, 29);
            }
        }


        planetPtr->draw();
    }
}

glm::vec3 PlanetSystem::getEnvMapPosition() {
    return glm::vec3(env.matPosition[3]);
}