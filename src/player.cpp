#include "player.h"
#include "imgui/imgui.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
#include <GLFW/glfw3.h>

DISABLE_WARNINGS_PUSH()

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

DISABLE_WARNINGS_POP()

Player::Player(Window* pWindow, const glm::vec3 &pos, const glm::vec3 &forward, const glm::vec3 &up)
        : position(pos), forward(glm::normalize(forward)), m_pWindow(pWindow) {
}

void Player::updateInput() {
    constexpr float moveSpeed = 0.02f;

    glm::vec3 localMoveDelta{0};
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));
    if (m_pWindow->isKeyPressed(GLFW_KEY_A))
        position -= moveSpeed * right;
    if (m_pWindow->isKeyPressed(GLFW_KEY_D))
        position += moveSpeed * right;
    if (m_pWindow->isKeyPressed(GLFW_KEY_W))
        position += moveSpeed * forward;
    if (m_pWindow->isKeyPressed(GLFW_KEY_S))
        position -= moveSpeed * forward;
    if (m_pWindow->isKeyPressed(GLFW_KEY_R))
        position += moveSpeed * up;
    if (m_pWindow->isKeyPressed(GLFW_KEY_F))
        position -= moveSpeed * up;
}