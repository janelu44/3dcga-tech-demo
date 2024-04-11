    //#include "Image.h"
#include "camera.h"
#include "player.h"
#include "mesh.h"
#include "texture.h"
#include "stb/stb_image.h"
#include "shadow/shadow_directions.h"
#include "planet_system.h"
#include "minimap/minimap.h"
#include "particle/engine.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()

#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
#include "bezier/path.h"

DISABLE_WARNINGS_POP()

#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>

class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(1600, 900), OpenGLVersion::GL45),
        m_texture("resources/textures/mars.jpg"),
        m_normalMap("resources/normal/mars_normal.jpg"),
        m_camera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
        m_playerCamera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
        m_firstCamera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
        m_thirdCamera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
        m_player(&m_window, INITIAL_POSITION, INITIAL_FORWARD) {
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods);
            });
        //        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
            });
        m_window.registerScrollCallback([&](glm::vec2 offset) {
            m_distance += offset.y * -0.1f;
            });
        m_window.registerWindowResizeCallback([&](const glm::ivec2& size) {
            m_projectionMatrix = glm::perspective(
                glm::radians(m_camera.fov),
                m_window.getAspectRatio(),
                m_camera.zNear,
                m_camera.zFar
            );
            });
        m_window.setMouseCapture(m_captureCursor);

        m_projectionMatrix = glm::perspective(
            glm::radians(m_camera.fov),
            m_window.getAspectRatio(),
            m_camera.zNear,
            m_camera.zFar
        );

        m_meshes = GPUMesh::loadMeshGPU("resources/meshes/iso_sphere.obj");
        m_cockpit = GPUMesh::loadMeshGPU("resources/meshes/cockpit_placeholder.obj");
        m_rocket = GPUMesh::loadMeshGPU("resources/meshes/rocket.obj");

        m_shadowMapFBO.Init(m_shadowMapSize, m_shadowMapSize);
        m_minimap.Init(m_minimapResolution, m_minimapResolution);

        loadCubemaps();

        try {
            ShaderBuilder testBuilder;
            testBuilder.addStage(GL_VERTEX_SHADER, "shaders/test_vert.glsl");
            testBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/test_frag.glsl");
            m_testShader = testBuilder.build();

            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            ShaderBuilder cubemapBuilder;
            cubemapBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            cubemapBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/cubemap_frag.glsl");
            m_cubemapShader = cubemapBuilder.build();

            ShaderBuilder reflectionBuilder;
            reflectionBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            reflectionBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/reflection_frag.glsl");
            m_reflectionShader = reflectionBuilder.build();

            ShaderBuilder minimapBuilder;
            minimapBuilder.addStage(GL_VERTEX_SHADER, "shaders/minimap_vert.glsl");
            minimapBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/minimap_frag.glsl");
            m_minimapShader = minimapBuilder.build();

            ShaderBuilder minimapColorBuilder;
            minimapColorBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            minimapColorBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/minimapColor_frag.glsl");
            m_minimapColorShader = minimapColorBuilder.build();

            ShaderBuilder particlesBuilder;
            particlesBuilder.addStage(GL_VERTEX_SHADER, "shaders/particles_vert.glsl");
            particlesBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/particles_frag.glsl");
            m_particlesShader = particlesBuilder.build();
        }
        catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void loadCubemaps() {
        for (int x = 0; x < m_skyboxImages.size(); x++) {
            GLuint texCubemap;
            glGenTextures(1, &texCubemap);
            glBindTexture(GL_TEXTURE_CUBE_MAP, texCubemap);

            int width, height, nrChannels;
            unsigned char* data;
            for (unsigned int i = 0; i < m_skyboxImages[x].size(); i++) {
                data = stbi_load(m_skyboxImages[x][i].c_str(), &width, &height, &nrChannels, 0);
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
                );
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            m_skyboxes.push_back(texCubemap);
        }

        std::vector<glm::vec3> skyboxVertices{
                {-1.0f, -1.0f, -1.0f},
                {1.0f,  -1.0f, -1.0f},
                {1.0f,  1.0f,  -1.0f},
                {-1.0f, 1.0f,  -1.0f},
                {-1.0f, -1.0f, 1.0f},
                {1.0f,  -1.0f, 1.0f},
                {1.0f,  1.0f,  1.0f},
                {-1.0f, 1.0f,  1.0f},
        };

        std::vector<glm::uvec3> skyboxTriangles{
                {0, 1, 2},
                {2, 3, 0},
                {1, 5, 6},
                {6, 2, 1},
                {5, 4, 7},
                {7, 6, 5},
                {4, 0, 3},
                {3, 7, 4},
                {3, 2, 6},
                {6, 7, 3},
                {0, 4, 5},
                {5, 1, 0}
        };
        GLuint vbo;
        glCreateBuffers(1, &vbo);
        glNamedBufferStorage(vbo, static_cast<GLsizeiptr>(skyboxVertices.size() * sizeof(glm::vec3)), skyboxVertices.data(), 0);

        GLuint ibo;
        glCreateBuffers(1, &ibo);
        glNamedBufferStorage(ibo, static_cast<GLsizeiptr>(skyboxTriangles.size() * sizeof(glm::uvec3)), skyboxTriangles.data(), 0);

        GLuint vao;
        glCreateVertexArrays(1, &vao);
        glVertexArrayElementBuffer(vao, ibo);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(glm::vec3));
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);

        m_cubemapVao = vao;
    }

    void renderCubeMap(const Shader& shader) {
        glm::mat4 mvpMatrix = m_projectionMatrix * glm::mat4(glm::mat3(m_viewMatrix));
        glDepthMask(GL_FALSE);
        shader.bind();
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(m_modelMatrix))));

        glBindVertexArray(m_cubemapVao);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxes[guiValues.skybox]);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(3 * 12), GL_UNSIGNED_INT, nullptr);
        glDepthMask(GL_TRUE);
    }

    void renderShadowMap() {
        glm::mat4 shadowProjectionMatrix = glm::perspective(
            glm::radians(90.0f),
            1.0f,
            m_camera.zNear,
            m_camera.zFar
        );

        glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
        glCullFace(GL_FRONT);
        glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

        for (SDir dir : ShadowDir::directions) {
            m_shadowMapFBO.BindForWriting(dir.cubemapFace);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            glm::mat4 lightViewMatrix = glm::lookAt(glm::vec3(0.0f), dir.forward, dir.up);
            glm::mat4 lightSpaceMatrix = shadowProjectionMatrix * lightViewMatrix;

            planetSystem.draw(lightSpaceMatrix, m_camera.position, m_shadowMapFBO, false, m_shadowShader, true, false);
            renderIoanSystem(m_shadowShader, lightSpaceMatrix);
            renderRocket(m_shadowShader, lightSpaceMatrix, false);
        }
    }

    void renderMinimapRocket(Shader& shader, glm::mat4 mvpMatrix) {
        for (GPUMesh& mesh : m_rocket) {
            shader.bind();

            glm::vec3 rocketColor = m_realisticMinimap ? glm::vec3(0.7f, 0.7f, 0.7f) : glm::vec3(1.0f, 0.1f, 0.1f);
            glm::vec3 rocketFwd = glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 playerFwdXZ = glm::normalize(glm::vec3(m_player.forward.x, 0.0f, m_player.forward.z));

            float angle = glm::acos(glm::dot(rocketFwd, playerFwdXZ));
            glm::vec3 rotationAxis = glm::normalize(glm::cross(rocketFwd, playerFwdXZ));

            glm::mat4 rocketPos = glm::translate(glm::mat4(1.0f), glm::vec3(m_player.position.x, 9.0f, m_player.position.z));
            glm::mat4 rocketRot = glm::rotate(rocketPos, angle, rotationAxis);
            glm::mat4 rocketScale = glm::scale(rocketRot, glm::vec3(0.25f + m_minimap.m_distance / 20.0f));
            glm::mat3 rocketNormal = glm::inverseTranspose(glm::mat3(rocketRot));

            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(rocketScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(rocketNormal));
            glUniform3fv(7, 1, glm::value_ptr(rocketColor));
            glUniform1i(8, GL_TRUE);
            glUniform1i(20, GL_FALSE);

            mesh.draw(shader);
        }
    }

    void renderMinimapTexture() {
        float resXY = float(m_minimap.m_resolution.x) / float(m_minimap.m_resolution.y);
        float resYX = float(m_minimap.m_resolution.y) / float(m_minimap.m_resolution.x);
        glm::mat4 minimapProjectionMatrix = glm::ortho(
            -0.005f * float(m_minimap.m_resolution.x) - m_minimap.m_distance * resXY,
            0.005f * float(m_minimap.m_resolution.x) + m_minimap.m_distance * resXY,
            -0.005f * float(m_minimap.m_resolution.y) - m_minimap.m_distance * resYX,
            0.005f * float(m_minimap.m_resolution.y) + m_minimap.m_distance * resYX,
            m_camera.zNear,
            m_camera.zFar);
        glm::vec3 minimapCenter = glm::vec3(m_player.position.x, 10.0f, m_player.position.z);
        glm::mat4 minimapViewMatrix = glm::lookAt(minimapCenter, minimapCenter + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        glm::mat4 minimapSpaceMatrix = minimapProjectionMatrix * minimapViewMatrix;

        m_minimap.BindForWriting();

        glCullFace(GL_BACK);
        glViewport(0, 0, m_minimap.m_resolution.x, m_minimap.m_resolution.y);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        planetSystem.draw(minimapSpaceMatrix, m_camera.position, m_shadowMapFBO, true, m_realisticMinimap ? m_defaultShader : m_minimapColorShader, true, true);
        renderIoanSystem(m_realisticMinimap ? m_defaultShader : m_minimapColorShader, minimapSpaceMatrix);
        renderMinimapRocket(m_realisticMinimap ? m_defaultShader : m_minimapColorShader, minimapSpaceMatrix);
    }

    void renderMinimap() {
        m_minimapShader.bind();

        glDisable(GL_DEPTH_TEST);

        glm::mat4 minimapPos = glm::translate(glm::mat4(1.0f), glm::vec3(guiValues.minimapPosition, 0.0f));
        glm::mat4 minimapScale = glm::scale(minimapPos, glm::vec3(guiValues.minimapScale, guiValues.minimapScale, 1.0f));

        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(minimapScale));
        m_minimap.BindForReading(GL_TEXTURE12);
        glUniform1i(2, 12);

        m_minimap.Draw();
        glEnable(GL_DEPTH_TEST);
    }

    void updateIoanSystem() {
        for (int i = 0; i < 3; i++)
            ioanProgress[i] = ioanPath.advance(ioanProgress[i], ioanSpeed * frametimeScale);
    }

    void renderIoanSystem(const Shader& shader, glm::mat4 mvpMatrix) {
        for (GPUMesh& mesh : m_meshes) {
            shader.bind();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glm::vec3 lightPos = glm::vec3(0.0f);
            glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
            glUniform3fv(6, 1, glm::value_ptr(lightPos));

            glUniform1i(20, m_shadowsEnabled);
            m_shadowMapFBO.BindForReading(GL_TEXTURE9);
            glUniform1i(21, 9);
            glUniform1f(22, 0.15f);

            // IOAN Alpha Beta Gamma
            glm::vec3 colors[3] = {
                {0.0f, 0.7f, 0.9f},
                {1.0f, 0.0f, 0.5f},
                {1.0f, 1.0f, 0.0f}
            };
            for (int i = 0; i < 3; i++) {
                glm::mat4 ioanPos = glm::mat4(1.0f);
                ioanPos = glm::translate(ioanPos, ioanPath.evaluate(ioanProgress[i]));
                ioanPos = glm::translate(ioanPos, glm::vec3(ioanCenter, 0, 0));
                glm::mat4 ioanNormal = glm::inverseTranspose(glm::mat3(ioanPos));
                glm::mat4 ioanScale = glm::scale(ioanPos, glm::vec3(ioanSize));

                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(ioanScale));
                glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(ioanNormal));
                glUniform3fv(7, 1, glm::value_ptr(colors[i]));
                glUniform1i(8, GL_FALSE);
                mesh.draw(shader);
            }
        }
    }

    void renderRocket(const Shader& shader, glm::mat4 mvpMatrix, bool renderCockpit = true) {
        glm::vec3 lightPos = glm::vec3(0.0f);

        for (GPUMesh& mesh : m_cockpit) {
            shader.bind();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix * m_firstCamera.viewMatrix() * glm::inverse(m_playerCamera.viewMatrix())));
            glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
            glUniform3fv(6, 1, glm::value_ptr(lightPos));

            glUniform1i(20, m_shadowsEnabled);
            m_shadowMapFBO.BindForReading(GL_TEXTURE9);
            glUniform1i(21, 9);
            glUniform1f(22, 0.01f);

            glm::mat3 cockpitNormal = glm::inverseTranspose(glm::mat3(m_modelMatrix));
            glm::mat4 cockpitScale = glm::scale(m_modelMatrix, glm::vec3(0.1f));

            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(cockpitScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(cockpitNormal));
            if (!m_thirdPerson && renderCockpit) mesh.draw(shader);
        }

        for (GPUMesh& mesh : m_rocket) {
            shader.bind();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
            glUniform3fv(6, 1, glm::value_ptr(lightPos));

            glUniform1i(20, m_shadowsEnabled);
            m_shadowMapFBO.BindForReading(GL_TEXTURE9);
            glUniform1i(21, 9);
            glUniform1f(22, 0.01f);

            glm::vec3 rocketFwd = glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 rocketUp = glm::vec3(0.0f, 1.0f, 0.0f);

            float angle = glm::acos(glm::dot(rocketFwd, m_player.forward));
            glm::vec3 rotationAxis = glm::normalize(glm::cross(rocketFwd, m_player.forward));

            glm::mat4 cockpitPos = glm::translate(m_modelMatrix, m_player.position);
            glm::mat4 cockpitRot = glm::rotate(cockpitPos, angle, rotationAxis);
            glm::mat3 cockpitNormal = glm::inverseTranspose(glm::mat3(cockpitRot));
            glm::mat4 cockpitScale = glm::scale(cockpitRot, glm::vec3(0.05f));

            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(cockpitScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(cockpitNormal));
            if (m_thirdPerson || !renderCockpit) mesh.draw(shader);

            if (m_thirdPerson)
                renderRocketExhaust(mvpMatrix, cockpitScale);
        }
    }

    void renderRocketExhaust(glm::mat4 mvpMatrix, glm::mat4 cockpitMatrix) {
        m_particlesShader.bind();

        glm::mat4 particlePos = glm::translate(cockpitMatrix, glm::vec3(0.0f, 0.0f, 1.3f));
        glm::mat4 particleScale = glm::scale(particlePos, glm::vec3(0.01f));

        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(particleScale));
        glUniform3fv(2, 1, glm::value_ptr(m_camera.up));
        glUniform3fv(3, 1, glm::value_ptr(glm::cross(m_camera.forward, m_camera.up)));
        glUniform2fv(4, 1, glm::value_ptr(glm::vec2(0.001f, 0.001f)));

        glm::vec3 particleColor{ 0.0f };
        if (m_player.moveForward.speed > 0.0f)
            particleColor = glm::vec3(1.0f, 0.0f, 0.0f);
        else
            particleColor = glm::vec3(0.0f, 0.0f, 1.0f);
        glUniform3fv(5, 1, glm::value_ptr(particleColor));

        glBindVertexArray(rocketExhaust.vao);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, rocketExhaust.billboard_vertex_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glVertexAttribDivisor(0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, rocketExhaust.particles_position_buffer);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glVertexAttribDivisor(1, 1);


        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, rocketExhaust.spawnedParticles);
    }
    
    void updateCamera() {
        m_playerCamera.update(m_captureCursor && !m_detachedCamera, frametime);
        m_firstCamera.update(m_captureCursor && !m_thirdPerson, frametime);
        m_thirdCamera.update(m_captureCursor && m_thirdPerson, frametime);

        m_player.forward = m_playerCamera.forward;
        m_player.up = m_playerCamera.up;
        m_player.updateInput(frametime);

        m_playerCamera.position = m_player.position;
        m_firstCamera.position = m_player.position;
        m_thirdCamera.position = m_player.position - m_distance * m_thirdCamera.forward;
        m_camera = m_thirdPerson ? m_thirdCamera : m_firstCamera;
    }

    struct {
        int skybox = 0;
        const float sameLineOffset = 100.0f;

        // minimap
        glm::vec2 minimapPosition = glm::vec2(0.961f, 0.413f);
        float minimapScale = 0.5f;
    } guiValues;

    void gui() {
        float sameLineOffset = 100.0f;
        ImGui::Begin("Debug");

        ImGui::BeginTabBar("#tab_bar");
        if (ImGui::BeginTabItem("Planet system")) {
            planetSystem.drawGui();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::Text("Minimap position");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat2("##MinimapPosition", glm::value_ptr(guiValues.minimapPosition), 0.001f);

            ImGui::Text("Minimap scale");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat("##MinimapScale", &guiValues.minimapScale, 0.001f);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        ImGui::End();
    }

    void update() {
        loadCubemaps();

        while (!m_window.shouldClose()) {
            const long long currentFrameTimestamp = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
            if (lastFrameTimestamp != -1) {
                frametime = currentFrameTimestamp - lastFrameTimestamp;
                frametimeScale = frametime / 16.67f; // scale for preserving 60fps movement in old code
            }
            lastFrameTimestamp = currentFrameTimestamp;

            m_window.updateInput();
            gui();

            planetSystem.update();
            updateIoanSystem();
            updateCamera();
            rocketExhaust.update(frametime, std::max(std::min(abs(m_player.moveForward.speed), 0.05f) / 0.05f, 0.01f));

            // Update projection and mvp matrices
            m_projectionMatrix = glm::perspective(
                glm::radians(m_camera.fov),
                m_window.getAspectRatio(),
                m_camera.zNear,
                m_camera.zFar
            );
            m_viewMatrix = m_camera.viewMatrix();
            glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix;

            if (m_shadowsEnabled) renderShadowMap();
            if (m_minimapEnabled) renderMinimapTexture();

            // Set Framebuffer settings
            glCullFace(GL_BACK);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, m_window.getWindowSize().x, m_window.getWindowSize().y);

            // Clear the screen
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            // Renders
            renderCubeMap(m_cubemapShader);
            planetSystem.draw(mvpMatrix, m_camera.position, m_shadowMapFBO, true, m_shadowShader, false, true);
            renderIoanSystem(m_defaultShader, mvpMatrix);
            renderRocket(m_defaultShader, mvpMatrix);
            if (m_minimapEnabled) renderMinimap();

            m_window.swapBuffers();
        }
    }

    void onKeyPressed(int key, int mods) {
        if (key == GLFW_KEY_C) {
            m_captureCursor = !m_captureCursor;
            m_window.setMouseCapture(m_captureCursor);
        }
        if (key == GLFW_KEY_V) {
            m_thirdPerson = !m_thirdPerson;
            m_firstCamera = m_playerCamera;
            m_thirdCamera = m_playerCamera;
        }
        if (key == GLFW_KEY_H) {
            m_shadowsEnabled = !m_shadowsEnabled;
        }
        if (key == GLFW_KEY_M) {
            m_minimapEnabled = !m_minimapEnabled;
        }
        if (key == GLFW_KEY_N) {
            m_realisticMinimap = !m_realisticMinimap;
        }
        if (key == GLFW_KEY_EQUAL) {
            if (m_minimap.m_distance > 2.0f) m_minimap.m_distance -= 1.0f;
        }
        if (key == GLFW_KEY_MINUS) {
            m_minimap.m_distance += 1.0f;
        }
    }

    void onKeyReleased(int key, int mods) {
    }

    void onMouseMove(const glm::dvec2& cursorPos) {
    }

    void onMouseClicked(int button, int mods) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            m_detachedCamera = true;
        }
    }

    void onMouseReleased(int button, int mods) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            m_detachedCamera = false;
            if (m_thirdPerson)
                m_thirdCamera.setTarget(m_playerCamera.forward, m_playerCamera.up, 100.0f);
            else
                m_firstCamera.setTarget(m_playerCamera.forward, m_playerCamera.up, 100.0f);
        }
    }

private:
    long long frametime{ 0 };
    float frametimeScale{ 0 };
    long long lastFrameTimestamp{ -1 };

    const glm::vec3 INITIAL_POSITION = glm::vec3(1.2f, 1.1f, 0.9f) * 5.0f;
    const glm::vec3 INITIAL_FORWARD = glm::vec3(-5.0f, -10.0f, 0.5f);

    Window m_window;

    Player m_player;
    Camera m_playerCamera;

    Camera m_firstCamera;
    Camera m_thirdCamera;
    Camera m_camera;

    // Camera settings
    bool m_captureCursor{true};
    bool m_thirdPerson{false};
    float m_distance{1.0f};
    bool m_detachedCamera{false};

    // Shaders for default rendering and for depth rendering
    Shader m_testShader;
    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_cubemapShader;
    Shader m_reflectionShader;
    Shader m_minimapShader;
    Shader m_minimapColorShader;
    Shader m_particlesShader;

    // Shadow Mapping
    int m_shadowMapSize{8192}; // Higher resolution for better shadows at longer distances
    ShadowMapFBO m_shadowMapFBO;
    bool m_shadowsEnabled{true};
    bool m_realisticMinimap{false};

    // Minimap
    Minimap m_minimap;
    int m_minimapResolution{1024};
    bool m_minimapEnabled{true};

    std::vector<GPUMesh> m_meshes;
    std::vector<GPUMesh> m_cockpit;
    std::vector<GPUMesh> m_rocket;
    Texture m_texture;
    Texture m_normalMap;
    bool m_useMaterial{true};

    GLuint m_cubemapIbo;
    GLuint m_cubemapVbo;
    GLuint m_cubemapVao;
    std::vector<std::vector<std::string>> m_skyboxImages{
            {
                    "resources/textures/space/right.jpg",
                    "resources/textures/space/left.jpg",
                    "resources/textures/space/top.jpg",
                    "resources/textures/space/bottom.jpg",
                    "resources/textures/space/front.jpg",
                    "resources/textures/space/back.jpg"
            }
    };

    std::vector<GLuint> m_skyboxes;

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_modelMatrix{1.0f};

    PlanetSystem planetSystem;
    // de fapt ioan e sistem stabil format din trei corpuri de masa egala care orbiteaza in forma de 8
    float ioanCenter = 15.0f;
    float ioanSize = 0.4f;
    float ioanSpeed = 0.05f;
    float ioanProgress[3]{ 0.0f, 0.4f, 0.8f };
    BezierPath ioanPath{
        {
        // first segment startpoint + control
            {3.0f, 1.0f, -1.0f}, {4.0f, 1.5f, -1.0f},
        // first segment endpoint + control
            {4.0f, 0.0f, 1.0f}, {3.0f, 0.0f, 1.0f}
        },
        {
        // second segment endpoint + control
            {{-3.0f, 0.0f, -1.0f}, {-2.0f, 0.0f, -1.0f}},
        // third segment endpoint + control
            {{-3.0f, -1.0f, 1.0f}, {-4.0f, -0.5f, 1.0f}},
        },
        // fouth segment autofill
        true
    };

    ParticleEngine rocketExhaust{ 30000, 200 };
};

int main() {
    Application app;
    app.update();

    return 0;
}
