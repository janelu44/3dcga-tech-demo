//#include "Image.h"
#include "camera.h"
#include "player.h"
#include "mesh.h"
#include "texture.h"
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
              m_texture("resources/textures/checkerboard.png"),
              m_camera(&m_window, glm::vec3(1.2f, 1.1f, 0.9f) * 5.0f, -glm::vec3(0.0f, 0.0f, -1.0f)),
              m_player(&m_window, glm::vec3(1.2f, 1.1f, 0.9f) * 5.0f, -glm::vec3(0.0f, 0.0f, -1.0f)) {
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
            distance += offset.y * -0.1f;
        });
        m_window.registerWindowResizeCallback([&](const glm::ivec2& size) {
            glViewport(0, 0, size.x, size.y);
            m_projectionMatrix = glm::perspective(
                    glm::radians(m_camera.fov),
                    m_window.getAspectRatio(),
                    m_camera.zNear,
                    m_camera.zFar
            );
        });
        m_window.setMouseCapture(m_captureCursor);

        m_meshes = GPUMesh::loadMeshGPU("resources/meshes/sphere.obj");
        m_cockpit = GPUMesh::loadMeshGPU("resources/meshes/cockpit_placeholder.obj");
        m_rocket = GPUMesh::loadMeshGPU("resources/meshes/rocket/rocket.obj");

        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, "shaders/shadow_vert.glsl");
            m_shadowShader = shadowBuilder.build();
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void gui() {
        int dummyInteger = 0;
        ImGui::Begin("Window");
        ImGui::InputInt("This is an integer input",
                        &dummyInteger); // Use ImGui::DragInt or ImGui::DragFloat for larger range of numbers.
        ImGui::Text("Value is: %i", dummyInteger); // Use C printf formatting rules (%i is a signed integer)
        ImGui::Checkbox("Use material if no texture", &m_useMaterial);
        ImGui::End();
    }

    void update() {
        Planet sun{3.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f};
        Planet earth{1.0f, 7.0f, 0.0f, 0.0f, 1.0f, 0.3f};
        Planet moon{0.2f, 2.0f, 0.0f, 0.0f, 0.0f, 0.5f};

        while (!m_window.shouldClose()) {
            m_window.updateInput();
            gui();

            m_camera.updateInput(m_captureCursor);
            if (m_thirdPerson) {
                m_player.updateInput();
                m_camera.position = m_player.position - distance * m_camera.forward;
            } else {
                m_player.forward = m_camera.forward;
                m_player.up = m_camera.up;

                m_player.updateInput();
                m_camera.position = m_player.position;
            }

            earth.revolutionProgress += earth.revolutionSpeed;
            earth.orbitProgress += earth.orbitSpeed;

            moon.revolutionProgress += moon.revolutionSpeed;
            moon.orbitProgress += moon.orbitSpeed;

            m_projectionMatrix = glm::perspective(
                    glm::radians(m_camera.fov),
                    m_window.getAspectRatio(),
                    m_camera.zNear,
                    m_camera.zFar
            );
            m_viewMatrix = m_camera.viewMatrix();

            // Clear the screen
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html

            // Render meshes
            for (GPUMesh &mesh: m_meshes) {

                m_defaultShader.bind();
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                if (mesh.hasTextureCoords()) {
                    m_texture.bind(GL_TEXTURE0);
                    glUniform1i(3, 0);
                    glUniform1i(4, GL_TRUE);
                } else {
                    glUniform1i(4, GL_FALSE);
                }
                glm::vec3 lightPos = glm::vec3(0.0f);
                glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
                glUniform3fv(6, 1, glm::value_ptr(lightPos));

                // SUN
                glm::mat4 sunPos = glm::mat4(1.0f);
                glm::mat4 sunRot = glm::rotate(sunPos, glm::radians(sun.revolutionProgress), glm::vec3(0, 1, 0));
                glm::mat3 sunNormal = glm::inverseTranspose(glm::mat3(sunRot));
                glm::mat4 sunScale = glm::scale(sunRot, glm::vec3(sun.size));

                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(sunScale));
                glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(sunNormal));
                glUniform3fv(7, 1, glm::value_ptr(glm::vec3(1.0f, 0.5f, 0.0f)));
                glUniform1i(8, GL_TRUE);
                mesh.draw(m_defaultShader);

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
                glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.0f, 0.5f, 1.0f)));
                glUniform1i(8, GL_FALSE);
                mesh.draw(m_defaultShader);


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
                glUniform3fv(7, 1, glm::value_ptr(glm::vec3(0.7f, 0.7f, 0.7f)));
                glUniform1i(8, GL_FALSE);
                mesh.draw(m_defaultShader);
            }

            // Render cockpit meshes
            for (GPUMesh &mesh: m_cockpit) {
                m_defaultShader.bind();
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));
                if (mesh.hasTextureCoords()) {
                    m_texture.bind(GL_TEXTURE0);
                    glUniform1i(3, 0);
                    glUniform1i(4, GL_TRUE);
                } else {
                    glUniform1i(4, GL_FALSE);
                }
                glm::vec3 lightPos = glm::vec3(0.0f);
                glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
                glUniform3fv(6, 1, glm::value_ptr(lightPos));

                glm::mat3 cockpitNormal = glm::inverseTranspose(glm::mat3(m_modelMatrix));
                glm::mat4 cockpitScale = glm::scale(m_modelMatrix, glm::vec3(0.1f));

                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(cockpitScale));
                glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(cockpitNormal));
                glUniform1i(8, GL_TRUE);
                if (!m_thirdPerson) mesh.draw(m_defaultShader);
            }

            for (GPUMesh &mesh: m_rocket) {
                m_defaultShader.bind();
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                if (mesh.hasTextureCoords()) {
                    m_texture.bind(GL_TEXTURE0);
                    glUniform1i(3, 0);
                    glUniform1i(4, GL_TRUE);
                } else {
                    glUniform1i(4, GL_FALSE);
                }
                glm::vec3 lightPos = glm::vec3(0.0f);
                glUniform3fv(5, 1, glm::value_ptr(m_camera.position));
                glUniform3fv(6, 1, glm::value_ptr(lightPos));

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
                glUniform1i(8, GL_TRUE);
                if (m_thirdPerson) mesh.draw(m_defaultShader);
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
        }

    }

    void onKeyReleased(int key, int mods) {
    }

    void onMouseMove(const glm::dvec2 &cursorPos) {
    }

    void onMouseClicked(int button, int mods) {
//        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
//            m_detachedCamera = true;
//        }
    }

    void onMouseReleased(int button, int mods) {
//        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
//            m_detachedCamera = false;
//        }
    }

private:
    Window m_window;
    Camera m_camera;
    Player m_player;

    // Camera settings
    bool m_captureCursor{true};
    bool m_thirdPerson{false};
    float distance{1.0f};
//    bool m_detachedCamera{false};

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;

    std::vector<GPUMesh> m_meshes;
    std::vector<GPUMesh> m_cockpit;
    std::vector<GPUMesh> m_rocket;
    Texture m_texture;
    bool m_useMaterial{true};

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix{1.0f};
};

int main() {
    Application app;
    app.update();

    return 0;
}
