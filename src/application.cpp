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
#include "shadow/spotlight_shadow_map.h"

    DISABLE_WARNINGS_POP()

#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include "mazeify/generator.h"

struct ObjectRenderData {
    glm::vec3 position = glm::vec3(0.0f);
    float angle = 0.0f;
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 color = glm::vec3(0.0f);
    bool ignoreBehind = false;
};

class Application {
public:
    Application()
            : m_window("Final Project", glm::ivec2(1600, 900), OpenGLVersion::GL45),
              m_texture("resources/textures/mars.jpg"),
              m_normalMap("resources/normal/mars_normal.jpg"),
              m_camera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
              m_playerCamera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
              m_spacePlayerCamera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
              m_flatPlayerCamera(&m_window, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
              m_firstCamera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
              m_thirdCamera(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
              m_player(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
              m_spacePlayer(&m_window, INITIAL_POSITION, INITIAL_FORWARD),
              m_flatPlayer(&m_window, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
              m_tileTexture("resources/textures/grass_mossy.png"),
              m_mazeBlockTexture("resources/textures/leaves.png"),
              mazegen(12, 12) {
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
        m_window.registerWindowResizeCallback([&](const glm::ivec2 &size) {
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

        m_meshes = GPUMesh::loadMeshGPU("resources/meshes/rhino_sphere.obj");
        m_cockpit = GPUMesh::loadMeshGPU("resources/meshes/rhino_cockpit.obj");
        m_rocket = GPUMesh::loadMeshGPU("resources/meshes/rocket.obj");
        m_tile = GPUMesh::loadMeshGPU("resources/meshes/plane_tile.obj");
        m_character = GPUMesh::loadMeshGPU("resources/meshes/spong.obj");
        m_mazeBlock = GPUMesh::loadMeshGPU("resources/meshes/maze_block.obj");

        m_shadowMapFBO.Init(m_shadowMapSize, m_shadowMapSize);
        m_spotlightMap.Init(m_spotlightMapSize, m_spotlightMapSize);
        m_minimap.Init(m_minimapResolution, m_minimapResolution);
        m_envMap.Init(2048, 2048);

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

            ShaderBuilder materialBuilder;
            materialBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            materialBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/material_frag.glsl");
            m_materialShader = materialBuilder.build();

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
            reflectionBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/environment_frag.glsl");
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

            ShaderBuilder textureBuilder;
            textureBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            textureBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/texture_frag.glsl");
            m_textureShader = textureBuilder.build();
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
            unsigned char *data;
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
        glNamedBufferStorage(vbo, static_cast<GLsizeiptr>(skyboxVertices.size() * sizeof(glm::vec3)),
                             skyboxVertices.data(), 0);

        GLuint ibo;
        glCreateBuffers(1, &ibo);
        glNamedBufferStorage(ibo, static_cast<GLsizeiptr>(skyboxTriangles.size() * sizeof(glm::uvec3)),
                             skyboxTriangles.data(), 0);

        GLuint vao;
        glCreateVertexArrays(1, &vao);
        glVertexArrayElementBuffer(vao, ibo);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(glm::vec3));
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);

        m_cubemapVao = vao;
    }

    void renderCubeMap(const Shader &shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix, bool alt = false) {
        glm::mat4 mvpMatrix = projectionMatrix * glm::mat4(glm::mat3(viewMatrix));
        glDepthMask(GL_FALSE);
        shader.bind();
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(m_modelMatrix))));

        glBindVertexArray(m_cubemapVao);
        if (alt) {
            m_envMap.BindForReading(GL_TEXTURE28);
        } else {
            glActiveTexture(GL_TEXTURE28);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxes[guiValues.skybox]);
        }
        glUniform1i(70, 28);
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

        for (SDir dir: ShadowDir::directions) {
            m_shadowMapFBO.BindForWriting(dir.cubemapFace);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            glm::vec3 lightPos = m_flatWorld ? m_flatSunPos : glm::vec3(0.0f);
            glm::mat4 lightViewMatrix = glm::lookAt(lightPos, lightPos + dir.forward, dir.up);
            glm::mat4 lightSpaceMatrix = shadowProjectionMatrix * lightViewMatrix;

            if (m_flatWorld) {
                if (!m_isNight) renderFlatWorld(m_shadowShader, lightSpaceMatrix, true);
            } else {
                planetSystem.draw(lightSpaceMatrix, m_camera.position, m_shadowMapFBO, false, m_shadowShader, true,
                                  m_envMap, false);
                renderIoanSystem(m_shadowShader, lightSpaceMatrix);
                if (m_renderRocket) renderRocket(m_shadowShader, lightSpaceMatrix, false);
            }

        }
    }

    void renderEnvMap(glm::vec3 position) {
        glm::mat4 envProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 30.0f);

        glViewport(0, 0, 2048, 2048);
        glCullFace(GL_FRONT);
        glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

        for (SDir dir: ShadowDir::directions) {
            m_envMap.BindForWriting(dir.cubemapFace);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            glm::mat4 envViewMatrix = glm::lookAt(position, position + dir.forward, dir.up);
            glm::mat4 envSpaceMatrix = envProjectionMatrix * envViewMatrix;

            renderCubeMap(m_cubemapShader, envProjectionMatrix, envViewMatrix);
            planetSystem.draw(envSpaceMatrix, position, m_shadowMapFBO, true, m_shadowShader, false, m_envMap, false);
            renderIoanSystem(m_defaultShader, envSpaceMatrix);
            if (m_renderRocket) renderRocket(m_materialShader, envSpaceMatrix, false);
        }
    }

    void renderSpotlight() {
        glm::mat4 spotlightProjectionMatrix = glm::perspective(
                glm::radians(90.0f),
                1.0f,
                m_camera.zNear,
                m_camera.zFar
        );
        glm::mat4 lightViewMatrix = glm::lookAt(m_spotlightPos, m_spotlightPos + m_camera.forward, m_camera.up);
        glm::mat4 lightSpaceMatrix = spotlightProjectionMatrix * lightViewMatrix;

        m_spotlightMap.BindForWriting();
        glViewport(0, 0, m_spotlightMapSize, m_spotlightMapSize);
        glCullFace(GL_FRONT);
        glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        renderFlatWorld(m_shadowShader, lightSpaceMatrix, false, true);
    }

    void renderMinimapRocket(Shader& shader, glm::mat4 mvpMatrix) {
        for (GPUMesh& mesh : m_rocket) {
            shader.bind();

            glm::vec3 rocketColor = m_realisticMinimap ? glm::vec3(0.7f, 0.7f, 0.7f) : glm::vec3(1.0f, 0.1f, 0.1f);
            glm::vec3 rocketFwd = glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 playerFwdXZ = glm::normalize(glm::vec3(m_player.forward.x, 0.0f, m_player.forward.z));

            float angle = glm::acos(glm::dot(rocketFwd, playerFwdXZ));
            glm::vec3 rotationAxis = glm::normalize(glm::cross(rocketFwd, playerFwdXZ));

            glm::mat4 rocketPos = glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(m_player.position.x, 9.0f, m_player.position.z));
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
        glm::mat4 minimapViewMatrix = glm::lookAt(minimapCenter, minimapCenter + glm::vec3(0.0f, -1.0f, 0.0f),
                                                  m_thirdPerson ? glm::vec3(0.0f, 0.0f, -1.0f) : m_player.forward);
        glm::mat4 minimapSpaceMatrix = minimapProjectionMatrix * minimapViewMatrix;

        m_minimap.BindForWriting();

        glCullFace(GL_BACK);
        glViewport(0, 0, m_minimap.m_resolution.x, m_minimap.m_resolution.y);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        if (m_flatWorld) {
            glm::mat4 spotlightProjectionMatrix = glm::perspective(
                    glm::radians(90.0f),
                    1.0f,
                    m_camera.zNear,
                    m_camera.zFar
            );
            glm::mat4 lightViewMatrix = glm::lookAt(m_spotlightPos, m_spotlightPos + m_camera.forward, m_camera.up);
            glm::mat4 lightSpaceMatrix = spotlightProjectionMatrix * lightViewMatrix;
            renderFlatWorld(m_defaultShader, minimapSpaceMatrix, false, false, lightSpaceMatrix);
            renderMinimapRocket(m_realisticMinimap ? m_defaultShader : m_minimapColorShader, minimapSpaceMatrix);
        } else {
            planetSystem.draw(minimapSpaceMatrix, m_camera.position, m_shadowMapFBO, true, m_minimapColorShader,
                              !m_realisticMinimap, m_envMap, true);
            renderIoanSystem(m_realisticMinimap ? m_defaultShader : m_minimapColorShader, minimapSpaceMatrix);
            renderMinimapRocket(m_realisticMinimap ? m_defaultShader : m_minimapColorShader, minimapSpaceMatrix);
        }
    }

    void renderMinimap() {
        m_minimapShader.bind();

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE_MINUS_CONSTANT_ALPHA, GL_CONSTANT_ALPHA);
        glBlendColor(0.0f, 0.0f, 0.0f, m_thirdPerson || m_flatWorld ? 0.0f : 0.25f);

        glm::vec3 scale = m_thirdPerson || m_flatWorld ? glm::vec3(guiValues.minimapScaleTp, guiValues.minimapScaleTp, 1.0f) : glm::vec3(guiValues.minimapScaleFp, guiValues.minimapScaleFp, 1.0f);
        glm::vec3 pos = m_thirdPerson || m_flatWorld ? glm::vec3(guiValues.minimapPosition, 0.0f) : glm::vec3(0.0f, glm::mix(-0.139f, 0.009f, m_minimapFpvSlidePosition), guiValues.minimapPositionz);
        glm::mat4 minimapPos = glm::translate(glm::mat4(1.0f), pos);
        glm::mat4 minimapScale = glm::scale(minimapPos, scale);

        glm::mat4 mvp = m_thirdPerson || m_flatWorld ? m_projectionMatrix : m_projectionMatrix * m_firstCamera.viewMatrix() * glm::inverse(m_playerCamera.viewMatrix());
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(minimapScale));
        m_minimap.BindForReading(GL_TEXTURE4);
        glUniform1i(2, 4);

        m_minimap.Draw();
        glDisable(GL_BLEND);
    }

    void updateIoanSystem() {
        for (int i = 0; i < 3; i++)
            ioanProgress[i] = ioanPath.advance(ioanProgress[i], ioanSpeed * frametimeScale);
    }

    void renderIoanSystem(const Shader &shader, glm::mat4 mvpMatrix) {
        for (GPUMesh &mesh: m_meshes) {
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
            ioanCenter = glm::vec3(-5.0f, -7.0f, -5.0f);
            for (int i = 0; i < 3; i++) {
                glm::mat4 ioanPos = glm::mat4(1.0f);
                ioanPos = glm::translate(ioanPos, ioanPath.evaluate(ioanProgress[i]));
                ioanPos = glm::translate(ioanPos, ioanCenter);
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

    void renderRocket(const Shader &shader, glm::mat4 mvpMatrix, bool renderCockpit = true) {
        glm::vec3 lightPos = glm::vec3(0.0f);

        for (GPUMesh &mesh: m_cockpit) {
            shader.bind();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(
                    m_projectionMatrix * m_firstCamera.viewMatrix() * glm::inverse(m_playerCamera.viewMatrix())));
            glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
            glUniform3fv(6, 1, glm::value_ptr(lightPos));
            glUniform1i(8, GL_TRUE);

            glUniform1i(20, m_shadowsEnabled);
            m_shadowMapFBO.BindForReading(GL_TEXTURE9);
            glUniform1i(21, 9);
            glUniform1f(22, 0.01f);

            glm::mat4 cockpitRot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat3 cockpitNormal = glm::inverseTranspose(glm::mat3(cockpitRot));
            glm::mat4 cockpitScale = glm::scale(cockpitRot, glm::vec3(0.1f));

            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(cockpitScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(cockpitNormal));
            if (!m_thirdPerson && renderCockpit) mesh.draw(shader);
        }

        for (GPUMesh &mesh: m_rocket) {
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

    void renderFlatWorld(Shader& shader, glm::mat4 mvpMatrix, bool isShadowRender = false, bool isSpotlightRender = false, glm::mat4 lightMatrix = glm::mat4(1.0f)) {
        float genRadius = m_renderDistance;
        glm::vec2 genBottomCorner = glm::vec2(m_player.position.x - genRadius, m_player.position.z - genRadius);
        glm::vec2 genTopCorner = glm::vec2(m_player.position.x + genRadius, m_player.position.z + genRadius);

        std::vector<glm::vec3> tilePositions;
        for (float x = genBottomCorner.x; x < genTopCorner.x; x += 2.0f) {
            for (float y = genBottomCorner.y; y < genTopCorner.y; y += 2.0f) {
                glm::vec2 tileCenter = {std::round(x / 2.0f) * 2.0f, std::round(y / 2.0f) * 2.0f};
                glm::vec3 tilePos = glm::vec3(tileCenter.x, m_tileY, tileCenter.y);
                if (glm::distance(tilePos, m_player.position) < genRadius) tilePositions.push_back(tilePos);
            }
        }
        if (!isShadowRender && !isSpotlightRender) renderMultipleObjects(m_textureShader, m_tile, m_tileTexture, mvpMatrix, tilePositions, lightMatrix);

        glm::vec3 startingPos = m_mazeStartPos;
        std::vector<glm::vec3> mazeBlockPositions;
        for (int i = 0; i < 25; i++) {
            for (int j = 0; j < 25; j++) {
                if (m_mazeGrid[i][j] == 1) {
                    for (int k = 0; k < 3; k++) {
                        float factor = 0.24f;
                        glm::vec3 mazeBlockPos = startingPos + glm::vec3(i * factor, k * factor, j * factor);
                        if (glm::distance(mazeBlockPos, m_player.position) < genRadius) mazeBlockPositions.push_back(mazeBlockPos);
                    }
                }
            }
        }
        if (!isShadowRender && !isSpotlightRender) renderMultipleObjects(m_textureShader, m_mazeBlock, m_mazeBlockTexture, mvpMatrix, mazeBlockPositions, lightMatrix, true);

        // Sun render data
        ObjectRenderData sunData;
        sunData.position = m_flatSunPos;
        sunData.color = glm::vec3(0.6f, 0.6f, 0.1f);
        sunData.scale = glm::vec3(0.1f);
        sunData.ignoreBehind = GL_TRUE;
        if (!isShadowRender && !isSpotlightRender) renderFlatWorldObject(shader, m_meshes, mvpMatrix, sunData, lightMatrix);

        // Rocket render data
        ObjectRenderData rocketData;
        rocketData.position = m_flatRocketPos;
        rocketData.angle = glm::radians(90.0f);
        rocketData.rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        rocketData.color = glm::vec3(0.7f, 0.7f, 0.7f);
        rocketData.scale = glm::vec3(m_flatRocketScale);
        if (glm::distance(rocketData.position, m_player.position) < genRadius) {
            if (!isShadowRender && !isSpotlightRender) renderFlatWorldObject(m_materialShader, m_rocket, mvpMatrix, rocketData, lightMatrix, isSpotlightRender);
            else renderFlatWorldObject(shader, m_rocket, mvpMatrix, rocketData, lightMatrix, isSpotlightRender);
        }


        // Character render data
        ObjectRenderData characterData;
        glm::vec3 characterFwd = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 characterFwdXZ = glm::normalize(glm::vec3(m_player.forward.x, 0.0f, m_player.forward.z));
        characterData.position = m_player.position - glm::vec3(0.0f, m_flatCharacterY, 0.0f);
        characterData.angle = glm::acos(glm::dot(characterFwd, characterFwdXZ));
        characterData.rotationAxis = glm::normalize(glm::cross(characterFwd, characterFwdXZ));
        characterData.scale = glm::vec3(m_flatCharacterScale);
        if (isShadowRender && !isSpotlightRender) renderFlatWorldObject(shader, m_character, mvpMatrix, characterData, lightMatrix);
    }

    void renderFlatWorldObject(Shader& shader, std::vector<GPUMesh>& meshes, glm::mat4 mvpMatrix, ObjectRenderData data, glm::mat4 lightMatrix, bool isSpotlightRender = false) {
        for (GPUMesh &mesh: meshes) {
            shader.bind();

            glm::mat4 objectPos = glm::translate(glm::mat4(1.0f), data.position);
            glm::mat4 objectRot = glm::rotate(objectPos, data.angle, data.rotationAxis);
            glm::mat4 objectScale = glm::scale(objectRot, glm::vec3(data.scale));

            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(objectScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(objectRot))));
            glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
            glUniform3fv(6, 1, glm::value_ptr(isSpotlightRender ? m_spotlightPos : m_flatSunPos));
            glUniform3fv(7, 1, glm::value_ptr(data.color));
            glUniform1i(8, data.ignoreBehind);

            glUniform1i(20, m_shadowsEnabled);
            m_shadowMapFBO.BindForReading(GL_TEXTURE9);
            glUniform1i(21, 9);
            glUniform1f(22, 0.05f);
            m_spotlightMap.BindForReading(GL_TEXTURE8);
            glUniform1i(24, 8);
            glUniform1i(25, m_spotlightEnabled);
            glUniform3fv(26, 1, glm::value_ptr(m_spotlightPos));
            glUniformMatrix4fv(27, 1, GL_FALSE, glm::value_ptr(lightMatrix));
            glUniform1i(29, m_isNight);
            mesh.draw(shader);
        }
    }

    void renderMultipleObjects(Shader& shader, std::vector<GPUMesh>& meshes, Texture& tex, glm::mat4 mvpMatrix, std::vector<glm::vec3>& positions, glm::mat4 lightMatrix, bool ignoreBack = false) {
        for (GPUMesh &mesh: meshes) {
            shader.bind();

            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
            glUniform3fv(6, 1, glm::value_ptr(m_flatSunPos));
            glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.3f, 0.3f, 0.3f)));
            glUniform1i(8, ignoreBack);
            glUniform1i(20, m_shadowsEnabled);
            m_shadowMapFBO.BindForReading(GL_TEXTURE9);
            glUniform1i(21, 9);
            glUniform1f(22, 0.0005f);
            m_spotlightMap.BindForReading(GL_TEXTURE8);
            glUniform1i(24, 8);
            glUniform1i(25, m_spotlightEnabled);
            glUniform3fv(26, 1, glm::value_ptr(m_spotlightPos));
            glUniformMatrix4fv(27, 1, GL_FALSE, glm::value_ptr(lightMatrix));
            glUniform1i(29, m_isNight);
            tex.bind(GL_TEXTURE10);
            glUniform1i(30, 10);

            for (glm::vec3 pos: positions) {
                glm::mat4 tilePos = glm::translate(glm::mat4(1.0f), pos);
                glm::mat4 tileScale = glm::scale(tilePos, glm::vec3(1.0f));
                glm::mat3 tileNormal = glm::inverseTranspose(glm::mat3(tilePos));
                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(tileScale));
                glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(tileNormal));
                mesh.draw(shader);
            }
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
        else if (m_player.moveForward.speed < 0.0f)
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
        m_player.updateInput(frametime, m_flatWorld);

        m_playerCamera.position = m_player.position;
        m_firstCamera.position = m_player.position;
        m_thirdCamera.position = m_player.position - m_distance * m_thirdCamera.forward;
        m_camera = m_thirdPerson ? m_thirdCamera : m_firstCamera;

        m_spotlightPos = m_camera.position - glm::vec3(-0.1f, 0.05f, 0.0f);
    }

    void updateFlatWorldSun() {
        float angle = glm::radians(0.25f * frametimeScale);
        if (m_forceDay) m_flatSunPos = glm::vec3(0.0f, 30.0f, 0.0f);
        else if (m_forceNight) m_flatSunPos = glm::vec3(0.0f, -30.0f, 0.0f);
        else m_flatSunPos = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f)) * m_flatSunPos;
        m_isNight = m_flatSunPos.y <= -0.1f;

        glm::vec3 skyColorTop = glm::vec3(0.53f, 0.8f, 0.92f);
        glm::vec3 skyColorBot = glm::vec3(0.053f, 0.08f, 0.092f);
        glm::vec3 skyColorNight = glm::vec3(0.05f, 0.05f, 0.1f);

        float skyRatio = (m_flatSunPos.y - 3.0f) / 27.0f;
        if (m_flatSunPos.y > 3.0f) m_skyColor = skyRatio * skyColorTop + (1.0f - skyRatio) * skyColorBot;
        else if (m_flatSunPos.y >= 0.0f) m_skyColor = (m_flatSunPos.y / 3.0f) * skyColorBot +
                                                      (1.0f - m_flatSunPos.y / 3.0f) * skyColorNight;
        else m_skyColor = skyColorNight;
    }

    struct {
        int skybox = 0;
        const float sameLineOffset = 100.0f;

        // minimap
        glm::vec2 minimapPosition = glm::vec2(0.97f, 0.423f);
        float minimapScaleFp = 0.138f;
        float minimapScaleTp = 0.5f;
        float minimapPositionz = 0.727f;
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
            ImGui::DragFloat("##MinimapPositioqn", &guiValues.minimapPositionz, 0.001f);
            ImGui::DragFloat("##MinimapSlide", &m_minimapFpvSlidePosition, 0.01f);

            ImGui::Text("Minimap scale");
            ImGui::SameLine(sameLineOffset);
            ImGui::DragFloat("##MinimapScale", &guiValues.minimapScaleTp, 0.001f);
            ImGui::DragFloat("##MinimapScaleq", &guiValues.minimapScaleFp, 0.001f);
            ImGui::SliderFloat("Render Distance", &m_renderDistance, 10.0f, 100.0f);

            ImGui::DragFloat3("##RocketPos", glm::value_ptr(m_flatRocketPos));
            ImGui::DragFloat("##RocketScale", &m_flatRocketScale);
            ImGui::DragFloat3("##MazePos", glm::value_ptr(m_mazeStartPos));
            ImGui::DragFloat("##CharacterScale", &m_flatCharacterScale);
            ImGui::DragFloat("##TileY", &m_tileY);
            ImGui::DragFloat("##Char", &m_flatCharacterY);


            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

        ImGui::End();
    }

    void update() {
        loadCubemaps();

        while (!m_window.shouldClose()) {
            const long long currentFrameTimestamp =
                    std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
            if (lastFrameTimestamp != -1) {
                frametime = currentFrameTimestamp - lastFrameTimestamp;
                frametimeScale = frametime / 16.67f; // scale for preserving 60fps movement in old code
            }
            lastFrameTimestamp = currentFrameTimestamp;

            m_window.updateInput();
            if (m_guiEnabled) gui();

            if (m_update) {
                updateFlatWorldSun();
                planetSystem.update();
                updateIoanSystem();
            }
            updateCamera();
            rocketExhaust.update(frametime, std::max(std::min(abs(m_player.moveForward.speed), 0.05f) / 0.05f, 0.01f));

            if (m_minimapEnabled && m_minimapFpvSlidePosition != 1.0f)
                m_minimapFpvSlidePosition = std::min(m_minimapFpvSlidePosition + frametime / 1000.f, 1.0f);
            else if (!m_minimapEnabled && m_minimapFpvSlidePosition != 0.0f)
                m_minimapFpvSlidePosition = std::max(m_minimapFpvSlidePosition - frametime / 1000.f, 0.0f);

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
            if (m_minimapEnabled || (!m_minimapEnabled && !m_flatWorld && !m_thirdPerson && m_minimapFpvSlidePosition != 0.0f)) renderMinimapTexture();
            if (m_spotlightEnabled && m_flatWorld) renderSpotlight();

            renderEnvMap(planetSystem.getEnvMapPosition());

            // Set Framebuffer settings
            glCullFace(GL_BACK);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, m_window.getWindowSize().x, m_window.getWindowSize().y);

            // Clear the screen
            glm::vec3 skyColor = m_flatWorld ? m_skyColor : glm::vec3(0.05f);
            glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            // Renders
            if (m_flatWorld) {
                glm::mat4 spotlightProjectionMatrix = glm::perspective(
                        glm::radians(90.0f),
                        1.0f,
                        m_camera.zNear,
                        m_camera.zFar
                );
                glm::mat4 lightViewMatrix = glm::lookAt(m_spotlightPos, m_spotlightPos + m_camera.forward, m_camera.up);
                glm::mat4 lightSpaceMatrix = spotlightProjectionMatrix * lightViewMatrix;

                renderFlatWorld(m_defaultShader, mvpMatrix, false, false, lightSpaceMatrix);
                if (m_minimapEnabled && !m_forceNight) renderMinimap();

            } else {
                renderCubeMap(m_cubemapShader, m_projectionMatrix, m_viewMatrix);
                planetSystem.draw(mvpMatrix, m_camera.position, m_shadowMapFBO, true, m_shadowShader, false, m_envMap,
                                  true);
                renderIoanSystem(m_defaultShader, mvpMatrix);
                if (m_renderRocket) renderRocket(m_materialShader, mvpMatrix);
                if (m_minimapEnabled || (!m_minimapEnabled && !m_thirdPerson && m_minimapFpvSlidePosition != 0.0f)) renderMinimap();
            }

            m_window.swapBuffers();
        }
    }

    void regenerateMaze() {
        const auto &maze = mazegen.generate();
        for (int i = 0; i < 25; i++)
            for (int j = 0; j < 25; j++)
                m_mazeGrid[i][j] = maze[i][j];
        m_mazeGrid[13][0] = 0;
        m_mazeGrid[13][24] = 0;
    }

    void onKeyPressed(int key, int mods) {
        if (key == GLFW_KEY_C) {
            m_captureCursor = !m_captureCursor;
            m_window.setMouseCapture(m_captureCursor);
        }
        if (key == GLFW_KEY_V) {
            if (m_flatWorld)
                return;

            m_thirdPerson = !m_thirdPerson;
            m_firstCamera = m_playerCamera;
            m_thirdCamera = m_playerCamera;
        }
        if (key == GLFW_KEY_H) {
            m_shadowsEnabled = !m_shadowsEnabled;
        }
        if (key == GLFW_KEY_G) {
            m_guiEnabled = !m_guiEnabled;
        }
        if (key == GLFW_KEY_E) {
            m_spotlightEnabled = !m_spotlightEnabled;
        }
        if (key == GLFW_KEY_1) {
            if (m_thirdPerson)
                return;

            if (!m_flatWorld)
                regenerateMaze();

            m_flatWorld = !m_flatWorld;

            (m_flatWorld ? m_spacePlayer : m_flatPlayer) = m_player;
            m_player = m_flatWorld ? m_flatPlayer : m_spacePlayer;

            (m_flatWorld ? m_spacePlayerCamera : m_flatPlayerCamera) = m_playerCamera;
            m_playerCamera = m_flatWorld ? m_flatPlayerCamera : m_spacePlayerCamera;
            m_playerCamera.resync();

            m_detachedCamera = false;
            m_firstCamera = m_playerCamera;
        }
        if (key == GLFW_KEY_M) {
            m_minimapEnabled = !m_minimapEnabled;
        }
        if (key == GLFW_KEY_N) {
            m_realisticMinimap = !m_realisticMinimap;
        }
        if (key == GLFW_KEY_EQUAL) {
            if (m_minimap.m_distance > 2.0f) m_minimap.m_distance -= 2.0f;
        }
        if (key == GLFW_KEY_MINUS) {
            m_minimap.m_distance += 2.0f;
        }
        if (key == GLFW_KEY_I) {
            m_forceDay = !m_forceDay;
            m_forceNight = false;
        }
        if (key == GLFW_KEY_O) {
            m_forceNight = !m_forceNight;
            m_forceDay = false;
        }
        if (key == GLFW_KEY_P) {
            m_update = !m_update;
        }
        if (key == GLFW_KEY_Z) {
            m_renderRocket = !m_renderRocket;
            m_minimapEnabled = m_renderRocket;
            m_minimapFpvSlidePosition = 0.0f;
        }
    }

    void onKeyReleased(int key, int mods) {
    }

    void onMouseMove(const glm::dvec2 &cursorPos) {
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
    long long frametime{0};
    float frametimeScale{0};
    long long lastFrameTimestamp{-1};

    const glm::vec3 INITIAL_POSITION = glm::vec3(1.2f, 1.1f, 0.9f) * 5.0f;
    const glm::vec3 INITIAL_FORWARD = glm::vec3(-5.0f, -10.0f, 0.5f);

    Window m_window;

    bool m_update{true};

    Player m_spacePlayer;
    Camera m_spacePlayerCamera;
    Player m_flatPlayer;
    Camera m_flatPlayerCamera;
    Camera m_playerCamera;
    Player m_player;

    Camera m_firstCamera;
    Camera m_thirdCamera;
    Camera m_camera;

    // Camera settings
    bool m_captureCursor{true};
    bool m_thirdPerson{false};
    float m_distance{1.0f};
    bool m_detachedCamera{false};

    bool m_guiEnabled{false};
    bool m_renderRocket{true};

    // Extra features
    bool m_flatWorld{false};
    float m_renderDistance{50.0f};
    glm::vec3 m_flatSunPos{0.0f, 30.0f, 0.0f};
    glm::vec3 m_skyColor{0.53f, 0.8f, 0.92f};
    bool m_isNight{false};
    bool m_forceDay{false};
    bool m_forceNight{false};
    glm::vec3 m_mazeStartPos{-1.0f, -0.2f, 3.0f};
    glm::vec3 m_flatRocketPos{-1.0f, 2.18f, -4.0f};
    float m_flatRocketScale{1.5f};
    float m_tileY{-0.2f};
    float m_flatCharacterScale{0.25f};
    float m_flatCharacterY{0.2f};

    // Shaders for default rendering and for depth rendering
    Shader m_testShader;
    Shader m_defaultShader;
    Shader m_materialShader;
    Shader m_shadowShader;
    Shader m_cubemapShader;
    Shader m_reflectionShader;
    Shader m_minimapShader;
    Shader m_minimapColorShader;
    Shader m_particlesShader;
    Shader m_textureShader;

    // Shadow Mapping
    int m_shadowMapSize{8192}; // Higher resolution for better shadows at longer distances
//    int m_shadowMapSize{12288};
    ShadowMapFBO m_shadowMapFBO;
    bool m_shadowsEnabled{true};
    bool m_realisticMinimap{false};
    SpotlightShadowMap m_spotlightMap;
    glm::vec3 m_spotlightPos{0.0f, 0.0f, 0.0f};
    int m_spotlightMapSize{1024};
    bool m_spotlightEnabled{true};

    // Environment mapping
    EnvMap m_envMap;

    // Minimap
    Minimap m_minimap;
    int m_minimapResolution{1024};
    float m_minimapFpvSlidePosition{ 0.0f };
    bool m_minimapEnabled{true};

    std::vector<GPUMesh> m_meshes;
    std::vector<GPUMesh> m_cockpit;
    std::vector<GPUMesh> m_rocket;
    std::vector<GPUMesh> m_tile;
    std::vector<GPUMesh> m_character;
    std::vector<GPUMesh> m_mazeBlock;
    Texture m_tileTexture;
    Texture m_mazeBlockTexture;
    Texture m_texture;
    Texture m_normalMap;
    bool m_useMaterial{true};

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

    MazeGenerator mazegen;
    int m_mazeGrid[25][25];

    std::vector<GLuint> m_skyboxes;

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_modelMatrix{1.0f};

    PlanetSystem planetSystem;
    // de fapt ioan e sistem stabil format din trei corpuri de masa egala care orbiteaza in forma de 8
    glm::vec3 ioanCenter = glm::vec3(10.0f);
    float ioanSize = 0.4f;
    float ioanSpeed = 0.05f;
    float ioanProgress[3]{0.0f, 0.4f, 0.8f};
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
