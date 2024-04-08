#include "camera.h"
#include "imgui/imgui.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
#include <GLFW/glfw3.h>

DISABLE_WARNINGS_PUSH()

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

DISABLE_WARNINGS_POP()

Camera::Camera(Window* pWindow, const glm::vec3 &pos, const glm::vec3 &forward)
        : position(pos), forward(glm::normalize(forward)), m_pWindow(pWindow) {
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position, position + forward, up);
}

void Camera::rotateX(float angle) {
    const glm::vec3 horAxis = glm::cross(s_yAxis, forward);

    forward = glm::normalize(glm::angleAxis(angle, horAxis) * forward);
    up = glm::normalize(glm::cross(forward, horAxis));
}

void Camera::rotateY(float angle) {
    const glm::vec3 horAxis = glm::cross(s_yAxis, forward);

    forward = glm::normalize(glm::angleAxis(angle, s_yAxis) * forward);
    up = glm::normalize(glm::cross(forward, horAxis));
}

void Camera::updateInput(bool captureCursor) {
    constexpr float lookSpeed = 0.0015f;

    const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
    const glm::vec2 delta = lookSpeed * glm::vec2(cursorPos - m_prevCursorPos);
    m_prevCursorPos = cursorPos;

    if (ImGui::GetIO().WantCaptureMouse) return;

    if (captureCursor) {
        if (delta.x != 0.0f)
            rotateY(-delta.x);
        if (delta.y != 0.0f)
            rotateX(-delta.y);
    }

}