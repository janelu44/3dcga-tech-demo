//#include "Image.h"
#include "camera.h"
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
#include "../framework/src/ImPlot/implot.h"


class Application {
public:
    Application()
            : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL45) {
        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();
            ShaderBuilder fourierBuilder;
            fourierBuilder.addStage(GL_VERTEX_SHADER, "shaders/fourier_vert.glsl");
            fourierBuilder.addStage(GL_FRAGMENT_SHADER, "shaders/fourier_frag.glsl");
            m_fourierShader = fourierBuilder.build();
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
        
        glCreateVertexArrays(1, &emptyVAO);

        glCreateTextures(GL_TEXTURE_2D, 1, &texDepth);
        glTextureStorage2D(texDepth, 1, GL_DEPTH_COMPONENT32F, 200, 200);
        glCreateTextures(GL_TEXTURE_2D, 1, &texColor);
        glTextureStorage2D(texColor, 1, GL_RGB8, 200, 200);
        glCreateFramebuffers(1, &fourierFramebuffer);
        glNamedFramebufferTexture(fourierFramebuffer, GL_DEPTH_ATTACHMENT, texDepth, 0);
        glNamedFramebufferTexture(fourierFramebuffer, GL_COLOR_ATTACHMENT0, texColor, 0);
    }

    void gui() {
        ImGui::Begin("Parameters");

        ImGui::Checkbox("Gabor/Phasor noise", &Gabor_Phasor_Toggle);
        ImGui::NewLine();

        ImGui::Checkbox("Show kernel centers", &Show_Kernel_Centers);
        ImGui::SliderInt("Kernels per cell", &Kernels_Per_Cell, 1, 64);
        ImGui::Checkbox("Show cells", &Show_Cell_Boundaries);
        ImGui::DragFloat("Cell size scale", &Cell_Size_Factor);
        ImGui::NewLine();

        ImGui::Checkbox("Gabor: Show Harmonic", &Gabor_Show_Harmonic);
        ImGui::NewLine();

        ImGui::Text("Gaussian");
        ImGui::DragFloat("Magnitude K", &Kernel_Gaussian_Magnitude, 0.01);
        if (ImGui::DragFloat("Bandwidth a", &Kernel_Gaussian_Bandwidth, 0.001)) {
            if (Kernel_Gaussian_Bandwidth < 0.001)
                Kernel_Gaussian_Bandwidth = 0.001;
        }
        ImGui::NewLine();

        ImGui::Text("Harmonic frequency");
        ImGui::DragFloat("Magnitude F_0", &Kernel_Harmonic_Frequency_Magnitude, 0.01);
        ImGui::DragFloat("Spread", &Kernel_Harmonic_Frequency_Magnitude_Spread, 0.0002);
        ImGui::NewLine();

        if (ImGui::DragFloat("Orientation omega_0", &Kernel_Harmonic_Frequency_Orientation, 0.05)) {
            if (Kernel_Harmonic_Frequency_Orientation < 0.0)
                Kernel_Harmonic_Frequency_Orientation = 0.0;
            if (Kernel_Harmonic_Frequency_Orientation > 180.0)
                Kernel_Harmonic_Frequency_Orientation = 180.0;
        }
        ImGui::DragFloat("Spread'", &Kernel_Harmonic_Frequency_Orientation_Spread, 0.5);
        ImGui::NewLine();

        ImGui::DragFloat("Scale", &Scale, 0.1);
        ImGui::Image((void*)(intptr_t)texColor, ImVec2(200, 200));

        ImGui::Combo("Profile", &profile, ".25% full\0.50% full\0sawtooth\0sine\0\0");

        static float xs1[1001], ys1[1001];
        for (int i = 0; i < 1001; ++i) {
            xs1[i] = i * 0.001f;
            float phi = glm::radians(360.0 * i / 1000);
            float pi = glm::pi<float>();
            if (profile == 0)
                ys1[i] = phi < pi / 2.0 ? 0.0 : 1.0;
            else if (profile == 1)
                ys1[i] = phi < pi ? 0.0 : 1.0;
            else if (profile == 2)
                ys1[i] = phi / (2.0 * pi);
            else
                ys1[i] = sin(phi);
        }
        if (ImPlot::BeginPlot("Oscillation profile")) {
            ImPlot::SetupAxes("x", "y", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
            ImPlot::PlotLine("f(x)", xs1, ys1, 1001);
            ImPlot::EndPlot();
        }

        ImGui::End();
    }

    void update() {
        while (!m_window.shouldClose()) {
            m_window.updateInput();
            gui();

            m_fourierShader.bind();
            glBindFramebuffer(GL_FRAMEBUFFER, fourierFramebuffer);

            glClear(GL_COLOR_BUFFER_BIT);
            glBindVertexArray(emptyVAO);

            glUniform1f(0, Kernel_Gaussian_Magnitude);
            glUniform1f(1, Kernel_Gaussian_Bandwidth);

            glUniform1f(2, Kernel_Harmonic_Frequency_Magnitude);
            glUniform1f(3, Kernel_Harmonic_Frequency_Magnitude_Spread);
            float alpha = glm::radians(Kernel_Harmonic_Frequency_Orientation);
            glUniform1f(4, alpha);
            float beta = glm::radians(Kernel_Harmonic_Frequency_Orientation_Spread);
            glUniform1f(5, beta);
            glUniform1f(6, Scale);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            m_defaultShader.bind();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glClear(GL_COLOR_BUFFER_BIT);
            glBindVertexArray(emptyVAO);

            glUniform1f(0, Kernel_Gaussian_Magnitude);
            glUniform1f(1, Kernel_Gaussian_Bandwidth);

            glUniform1f(2, Kernel_Harmonic_Frequency_Magnitude);
            glUniform1f(3, Kernel_Harmonic_Frequency_Magnitude_Spread);
            glUniform1f(4, alpha);
            glUniform1f(5, beta);

            glUniform1i(6, Gabor_Phasor_Toggle);

            glUniform1i(7, Kernels_Per_Cell);
            glUniform1i(8, Show_Cell_Boundaries);
            glUniform1f(9, Cell_Size_Factor);

            glUniform1i(10, Show_Kernel_Centers);
            glUniform1i(11, Gabor_Show_Harmonic);

            glUniform1i(12, profile);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods) {
        std::cout << "Key pressed: " << key << std::endl;
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
        std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods) {
        std::cout << "Released mouse button: " << button << std::endl;
    }

private:
    Window m_window;

    Shader m_defaultShader;
    Shader m_fourierShader;

    GLuint emptyVAO;
    
    GLuint fourierFramebuffer;
    GLuint texDepth, texColor;


    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix{1.0f};

    float Kernel_Gaussian_Magnitude{ 1.0 };
    float Kernel_Gaussian_Bandwidth{ 0.05 };
    float Kernel_Harmonic_Frequency_Magnitude{ 0.0625 };
    float Kernel_Harmonic_Frequency_Magnitude_Spread{ 0.0 };
    float Kernel_Harmonic_Frequency_Orientation{ 45.0 };
    float Kernel_Harmonic_Frequency_Orientation_Spread{ 0.0 };

    int Kernels_Per_Cell{ 16 };
    bool Show_Cell_Boundaries{ false };
    float Cell_Size_Factor { 1.0 };

    bool Gabor_Phasor_Toggle{ false };
    bool Show_Kernel_Centers{ false };

    bool Gabor_Show_Harmonic{ true };

    int profile{ 3 };

    float Scale{ 1.0 };
};

int main() {
    Application app;
    app.update();

    return 0;
}
