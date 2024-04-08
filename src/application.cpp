//#include "Image.h"
#include "camera.h"
#include "mesh.h"
#include "texture.h"
#include "stb/stb_image.h"
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
              m_camera(&m_window, glm::vec3(1.2f, 1.1f, 0.9f) * 5.0f, -glm::vec3(1.2f, 1.1f, 0.9f)) {
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
//            else if (action == GLFW_RELEASE)
//                onKeyReleased(key, mods);
        });
//        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
        });
        m_window.registerScrollCallback([&](glm::vec2 offset) {
            m_camera.zoom(offset.y);
        });
        m_window.registerWindowResizeCallback([&](const glm::ivec2 &size) {
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

        loadCubemap();

        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, "shaders/shadow_vert.glsl");
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
    }

    void loadCubemap() {
        GLuint texCubemap;
        glGenTextures(1, &texCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texCubemap);

        int width, height, nrChannels;
        unsigned char *data;
        for (unsigned int i = 0; i < m_cubemapFaces.size(); i++) {
            data = stbi_load(m_cubemapFaces[i].c_str(), &width, &height, &nrChannels, 0);
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

        m_texCubemap = texCubemap;

        std::vector<glm::vec3> skyboxVertices{
                {-1.0f, -1.0f, -1.0f},
                {1.0f, -1.0f, -1.0f},
                {1.0f, 1.0f, -1.0f},
                {-1.0f, 1.0f, -1.0f},
                {-1.0f, -1.0f, 1.0f},
                {1.0f, -1.0f, 1.0f},
                {1.0f, 1.0f, 1.0f},
                {-1.0f, 1.0f, 1.0f},
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
        m_cubemapVbo = vbo;

        GLuint ibo;
        glCreateBuffers(1, &ibo);
        glNamedBufferStorage(ibo, static_cast<GLsizeiptr>(skyboxTriangles.size() * sizeof(glm::uvec3)),skyboxTriangles.data(), 0);
        m_cubemapIbo = ibo;

        GLuint vao;
        glCreateVertexArrays(1, &vao);
        glVertexArrayElementBuffer(vao, ibo);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(glm::vec3));
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);

        m_cubemapVao = vao;
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

        loadCubemap();

        while (!m_window.shouldClose()) {
            m_window.updateInput();
            gui();
            m_camera.updateInput(m_captureCursor);
            if (!m_detachedCamera) m_lastCameraForward = m_camera.forward;

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

            glm::mat4 mvpMatrix = m_projectionMatrix * glm::mat4(glm::mat3(m_viewMatrix));
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html

            glDepthMask(GL_FALSE);
            m_cubemapShader.bind();
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
            glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(m_modelMatrix))));

            glBindVertexArray(m_cubemapVao);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_texCubemap);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(3 * 12), GL_UNSIGNED_INT, nullptr);
            glDepthMask(GL_TRUE);


            mvpMatrix = m_projectionMatrix * m_viewMatrix;

//            m_window.swapBuffers();
//            continue;

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
                float scale = m_thirdPerson ? 0.05f : 0.2f;
                glm::vec3 displacement = m_thirdPerson ? glm::vec3(-0.15f, -0.1f, -0.75f) : glm::vec3(0.0f);

                m_reflectionShader.bind();
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

//                // COCKPIT PLACEHOLDER -> BANANA ROTATE (NU MERGE BINE INCA DAR NU MAI POT SA MA MAI UIT)
//                glm::vec3 cockpitDir = glm::vec3(0, 0, -1);
//                glm::vec3 cameraDir = glm::normalize(m_lastCameraForward);
//
//                float angle = glm::acos(glm::dot(cockpitDir, cameraDir));
//                glm::vec3 rotationAxis = glm::normalize(glm::cross(cockpitDir, cameraDir));
//
//                glm::mat4 cockpitPos = glm::translate(glm::mat4(1.0f), m_camera.position);
//                glm::mat4 cockpitRot = glm::rotate(cockpitPos, angle, rotationAxis);
//                glm::mat4 cockpitDisplaced = glm::translate(cockpitRot, displacement);
//                glm::mat3 cockpitNormal = glm::inverseTranspose(glm::mat3(cockpitDisplaced));
//                glm::mat4 cockpitScale = glm::scale(cockpitDisplaced, glm::vec3(scale));

                glm::mat4 cockpitPos = glm::translate(m_modelMatrix, displacement);

                glm::mat3 cockpitNormal = glm::inverseTranspose(glm::mat3(cockpitPos));
                glm::mat4 cockpitScale = glm::scale(cockpitPos, glm::vec3(scale));

                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(cockpitScale));
                glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(cockpitNormal));
                glBindTexture(GL_TEXTURE_CUBE_MAP, m_texCubemap);
                glUniform1i(8, GL_TRUE);
                mesh.draw(m_reflectionShader);
            }

            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods) {
//        std::cout << "Key pressed: " << key << std::endl;
        if (key == GLFW_KEY_C) {
            m_captureCursor = !m_captureCursor;
            m_window.setMouseCapture(m_captureCursor);
        }
        if (key == GLFW_KEY_V) {
            m_thirdPerson = !m_thirdPerson;
            if (m_thirdPerson) m_camera.position -= glm::vec3(-0.15f, -0.1f, -0.75f);
            else m_camera.position += glm::vec3(-0.15f, -0.1f, -0.75f);
        }

    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods) {
        std::cout << "Key released: " << key << std::endl;
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2 &cursorPos) {
        std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods) {
//        std::cout << "Pressed mouse button: " << button << std::endl;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            m_detachedCamera = true;
        }
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods) {
//        std::cout << "Released mouse button: " << button << std::endl;
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            m_detachedCamera = false;
        }
    }

private:
    Window m_window;
    Camera m_camera;

    // Camera settings
    bool m_captureCursor{false};
    bool m_thirdPerson{false};
    bool m_detachedCamera{false};
    glm::vec3 m_lastCameraForward{0.0f, 0.0f, -1.0f};

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_cubemapShader;
    Shader m_reflectionShader;

    std::vector<GPUMesh> m_meshes;
    std::vector<GPUMesh> m_cockpit;
    Texture m_texture;
    bool m_useMaterial{true};

    GLuint m_texCubemap;
    GLuint m_cubemapIbo;
    GLuint m_cubemapVbo;
    GLuint m_cubemapVao;
    std::vector<std::string> m_cubemapFaces
            {
                    "resources/textures/skybox/right.jpg",
                    "resources/textures/skybox/left.jpg",
                    "resources/textures/skybox/top.jpg",
                    "resources/textures/skybox/bottom.jpg",
                    "resources/textures/skybox/front.jpg",
                    "resources/textures/skybox/back.jpg"
            };

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
