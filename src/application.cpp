    //#include "Image.h"
#include "camera.h"
#include "player.h"
#include "mesh.h"
#include "texture.h"
#include "stb/stb_image.h"
#include "shadow/shadow_directions.h"
#include "shadow/shadow_map_fbo.h"
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
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>

DISABLE_WARNINGS_POP()

#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include <glm/gtx/quaternion.hpp>
#include <cmath>

struct Planet {
    float size = 1.0f;
    float orbitSize = 2.0f;
    float revolutionProgress = 0.0f;
    float orbitProgress = 0.0f;
    float revolutionSpeed = 1.0f;
    float orbitSpeed = 1.0f;
};

class Application {
public:
    Application()
            : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL45),
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
              m_flatPlayer(&m_window, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)) {
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

        m_meshes = GPUMesh::loadMeshGPU("resources/meshes/iso_sphere.obj");
        m_cockpit = GPUMesh::loadMeshGPU("resources/meshes/cockpit_placeholder.obj");
        m_rocket = GPUMesh::loadMeshGPU("resources/meshes/rocket/rocket.obj");
        m_tile = GPUMesh::loadMeshGPU("resources/meshes/flat_tile.obj");

        m_shadowMapFBO.Init(m_shadowMapSize, m_shadowMapSize);

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

        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }

        sun = {3.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f};
        earth = {1.0f, 7.0f, 0.0f, 0.0f, 1.0f, 0.3f};
        moon = {0.2f, 2.0f, 0.0f, 0.0f, 0.0f, 0.5f};
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
        glNamedBufferStorage(vbo, static_cast<GLsizeiptr>(skyboxVertices.size() * sizeof(glm::vec3)), skyboxVertices.data(), 0);

        GLuint ibo;
        glCreateBuffers(1, &ibo);
        glNamedBufferStorage(ibo, static_cast<GLsizeiptr>(skyboxTriangles.size() * sizeof(glm::uvec3)),skyboxTriangles.data(), 0);

        GLuint vao;
        glCreateVertexArrays(1, &vao);
        glVertexArrayElementBuffer(vao, ibo);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(glm::vec3));
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);

        m_cubemapVao = vao;
    }

    void renderCubeMap(const Shader &shader) {
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
                m_window.getAspectRatio(),
                m_camera.zNear,
                m_camera.zFar
        );

        glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
        glCullFace(GL_FRONT);
        glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

        for (SDir dir : ShadowDir::directions) {
            m_shadowMapFBO.BindForWriting(dir.cubemapFace);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            glm::mat4 m_lightViewMatrix = glm::lookAt(glm::vec3(0.0f), dir.forward, dir.up);
            glm::mat4 m_lightSpaceMatrix = shadowProjectionMatrix * m_lightViewMatrix;

            if (m_flatWorld) {

            } else {
                renderSolarSystem(m_shadowShader, m_lightSpaceMatrix, false);
                renderRocket(m_shadowShader, m_lightSpaceMatrix, false);
            }
        }
    }

    void updateSolarSystem() {
        earth.revolutionProgress += earth.revolutionSpeed;
        earth.orbitProgress += earth.orbitSpeed;

        moon.revolutionProgress += moon.revolutionSpeed;
        moon.orbitProgress += moon.orbitSpeed;
    }

    void renderSolarSystem(const Shader& shader, glm::mat4 mvpMatrix, bool renderSun = true) {
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

            // SUN
            glm::mat4 sunPos = glm::mat4(1.0f);
            glm::mat4 sunRot = glm::rotate(sunPos, glm::radians(sun.revolutionProgress), glm::vec3(0, 1, 0));
            glm::mat3 sunNormal = glm::inverseTranspose(glm::mat3(sunRot));
            glm::mat4 sunScale = glm::scale(sunRot, glm::vec3(sun.size));

            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(sunScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(sunNormal));
            glUniform3fv(7, 1, glm::value_ptr(glm::vec3(1.0f, 0.5f, 0.0f)));
            glUniform1i(8, GL_TRUE);
            if (renderSun) mesh.draw(shader);

            // EARTH
            glm::mat4 earthPos = sunPos;
            earthPos = glm::rotate(earthPos, glm::radians(earth.orbitProgress), glm::vec3(0, 1, 0));
            earthPos = glm::translate(earthPos, glm::vec3(earth.orbitSize, 0, 0));
            glm::mat4 earthRot = glm::rotate(earthPos, glm::radians(earth.revolutionProgress - earth.orbitProgress),
                                             glm::vec3(0, 1, 0));
            glm::mat3 earthNormal = glm::inverseTranspose(glm::mat3(earthRot));
            glm::mat4 earthScale = glm::scale(earthRot, glm::vec3(earth.size));

            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(earthScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(earthNormal));
            m_texture.bind(GL_TEXTURE0);
            glUniform1i(3, 0);
            glUniform1i(4, GL_TRUE);
            glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.0f, 0.5f, 1.0f)));
            glUniform1i(8, GL_FALSE);
            m_normalMap.bind(GL_TEXTURE1);
            glUniform1i(9, 1);
            mesh.draw(shader);

            // MOON
            glm::mat4 moonPos = earthPos;
            moonPos = glm::rotate(moonPos, glm::radians(moon.orbitProgress), glm::vec3(0, 1, 0));
            moonPos = glm::translate(moonPos, glm::vec3(moon.orbitSize, 0, 0));
            glm::mat4 moonRot = glm::rotate(moonPos, glm::radians(moon.revolutionProgress - moon.orbitProgress),
                                            glm::vec3(0, 1, 0));
            glm::mat4 moonNormal = glm::inverseTranspose(glm::mat3(moonRot));
            glm::mat4 moonScale = glm::scale(moonRot, glm::vec3(moon.size));

            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(moonScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(moonNormal));
            glUniform1i(4, GL_FALSE);
            glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.7f, 0.7f, 0.7f)));
            glUniform1i(8, GL_FALSE);

            mesh.draw(shader);
        }
    }

    void renderRocket(const Shader& shader, glm::mat4 mvpMatrix, bool renderCockpit = true) {
        glm::vec3 lightPos = glm::vec3(0.0f);

        for (GPUMesh &mesh: m_cockpit) {
            shader.bind();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix * m_firstCamera.viewMatrix() * glm::inverse(m_playerCamera.viewMatrix())));
            if (mesh.hasTextureCoords()) {
                m_texture.bind(GL_TEXTURE0);
                glUniform1i(3, 0);
                glUniform1i(4, GL_TRUE);
            } else {
                glUniform1i(4, GL_FALSE);
            }
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

        for (GPUMesh &mesh: m_rocket) {
            shader.bind();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            if (mesh.hasTextureCoords()) {
                m_texture.bind(GL_TEXTURE0);
                glUniform1i(3, 0);
                glUniform1i(4, GL_TRUE);
            } else {
                glUniform1i(4, GL_FALSE);
            }
            glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
            glUniform3fv(6, 1, glm::value_ptr(lightPos));

            glUniform1i(20, m_shadowsEnabled);
            m_shadowMapFBO.BindForReading(GL_TEXTURE9);
            glUniform1i(21, 9);
            glUniform1f(22, 0.01f);

            glm::vec3 rocketFwd = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 rocketUp = glm::vec3(0.0f, 0.0f, 1.0f);

            float angle = glm::acos(glm::dot(rocketFwd, m_player.forward));
            glm::vec3 rotationAxis = glm::normalize(glm::cross(rocketFwd, m_player.forward));

            glm::mat4 cockpitPos = glm::translate(m_modelMatrix, m_player.position);
            glm::mat4 cockpitRot = glm::rotate(cockpitPos, angle, rotationAxis);
            glm::mat3 cockpitNormal = glm::inverseTranspose(glm::mat3(cockpitRot));
            glm::mat4 cockpitScale = glm::scale(cockpitRot, glm::vec3(0.05f));

            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(cockpitScale));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(cockpitNormal));
//            glActiveTexture(GL_TEXTURE4);
//            glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxes[guiValues.skybox]);
//            glUniform1i(7, 4);
//            glUniform1i(8, GL_TRUE);
            if (m_thirdPerson || !renderCockpit) mesh.draw(shader);
        }
    }

    void renderFlatWorld(Shader& shader, glm::mat4 mvpMatrix) {
        float genRadius = m_renderDistance;
        glm::vec2 genBottomCorner = glm::vec2(m_player.position.x - genRadius, m_player.position.z - genRadius);
        glm::vec2 genTopCorner = glm::vec2(m_player.position.x + genRadius, m_player.position.z + genRadius);

        for (float x = genBottomCorner.x; x < genTopCorner.x; x += 2.0f) {
            for (float y = genBottomCorner.y; y < genTopCorner.y; y += 2.0f) {
                glm::vec2 tileCenter = {std::round(x / 2.0f) * 2.0f, std::round(y / 2.0f) * 2.0f};
                glm::vec3 tilePos = glm::vec3(tileCenter.x, 0.0f, tileCenter.y);
                if (glm::distance(tilePos, m_player.position) < genRadius) renderFlatTile(shader, mvpMatrix, tilePos);
            }
        }
    }

    void renderFlatTile(Shader& shader, glm::mat4 mvpMatrix, glm::vec3 position) {
        for (GPUMesh &mesh: m_tile) {
            shader.bind();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), position - glm::vec3(0.0f, 0.1f, 0.0f))));
            glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(glm::mat4(1.0f)))));
            glUniform3fv(6, 1, glm::value_ptr(glm::vec3(0.0f, 7.0f, 0.0f)));
            glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.3f, 0.3f, 0.3f)));
            glUniform1i(8, GL_TRUE);
            glUniform1i(20, GL_FALSE);
            mesh.draw(shader);
        }
    }

    void updateCamera() {
        m_camera.updateInput(m_captureCursor);
        m_playerCamera.updateInput(m_captureCursor && !m_detachedCamera);
        m_firstCamera.updateInput(m_captureCursor && !m_thirdPerson);
        m_thirdCamera.updateInput(m_captureCursor && m_thirdPerson);

        m_player.forward = m_playerCamera.forward;
        m_player.up = m_playerCamera.up;
        m_player.updateInput();

        m_playerCamera.position = m_player.position;
        m_firstCamera.position = m_player.position;
        m_thirdCamera.position = m_player.position - m_distance * m_thirdCamera.forward;
        m_camera = m_thirdPerson ? m_thirdCamera : m_firstCamera;
    }

    struct {
        int skybox = 0;
        const float sameLineOffset = 100.0f;
    } guiValues;

    void gui() {
        ImGui::Begin("Debug");
        const char *skyboxOptions[] = {"Space", "Lake"};
        ImGui::Text("Skybox");
        ImGui::SameLine(guiValues.sameLineOffset);
        ImGui::Combo("##Skybox", &guiValues.skybox, skyboxOptions, 2);

        ImGui::SliderFloat("Render Distance", &m_renderDistance, 10.0f, 100.0f);

        ImGui::End();
    }

    void update() {
        loadCubemaps();

        while (!m_window.shouldClose()) {
            m_window.updateInput();
            gui();

            updateSolarSystem();
            updateCamera();

            // Update projection and mvp matrices
            m_projectionMatrix = glm::perspective(
                    glm::radians(m_camera.fov),
                    m_window.getAspectRatio(),
                    m_camera.zNear,
                    m_camera.zFar
            );
            m_viewMatrix = m_camera.viewMatrix();
            glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix;

            if(m_shadowsEnabled) renderShadowMap();

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
            if (m_flatWorld) {
                renderFlatWorld(m_defaultShader, mvpMatrix);
            } else {
                renderSolarSystem(m_defaultShader, mvpMatrix);
                renderRocket(m_defaultShader, mvpMatrix);
            }

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
        if (key == GLFW_KEY_1) {
            m_flatWorld = !m_flatWorld;
            m_playerCamera = m_flatWorld ? m_flatPlayerCamera : m_spacePlayerCamera;
            m_player = m_flatWorld ? m_flatPlayer : m_spacePlayer;
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
            m_firstCamera = m_playerCamera;
            m_thirdCamera = m_playerCamera;
        }
    }

private:
    const glm::vec3 INITIAL_POSITION = glm::vec3(1.2f, 1.1f, 0.9f) * 5.0f;
    const glm::vec3 INITIAL_FORWARD = glm::vec3(-5.0f, -10.0f, 0.5f);

    Window m_window;

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

    // Extra features
    bool m_flatWorld{false};
    float m_renderDistance{40.0f};

    // Shader for default rendering and for depth rendering
    Shader m_testShader;
    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_cubemapShader;
    Shader m_reflectionShader;

    // Shadow Mapping
    int m_shadowMapSize{8192}; // Higher resolution for better shadows at longer distances
    ShadowMapFBO m_shadowMapFBO;
    bool m_shadowsEnabled{true};

    std::vector<GPUMesh> m_meshes;
    std::vector<GPUMesh> m_cockpit;
    std::vector<GPUMesh> m_rocket;
    std::vector<GPUMesh> m_tile;
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

    Planet sun, earth, moon;
};

int main() {
    Application app;
    app.update();

    return 0;
}
