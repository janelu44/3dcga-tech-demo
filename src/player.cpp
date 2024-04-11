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

void Player::updateInput(long long frametime, bool flatMovement) {
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));

    const auto& dirHelper = [](bool fwd, bool bck) {
        if (!(fwd xor bck))
            return 0;
        return fwd ? 1 : -1;
        };

    moveRight.update(
        dirHelper(
            m_pWindow->isKeyPressed(GLFW_KEY_D),
            m_pWindow->isKeyPressed(GLFW_KEY_A)
        ),
        frametime);

    moveForward.update(
        dirHelper(
            m_pWindow->isKeyPressed(GLFW_KEY_W),
            m_pWindow->isKeyPressed(GLFW_KEY_S)
        ),
        frametime);

    moveUp.update(
        dirHelper(
            m_pWindow->isKeyPressed(GLFW_KEY_R) || m_pWindow->isKeyPressed(GLFW_KEY_SPACE),
            m_pWindow->isKeyPressed(GLFW_KEY_F) || m_pWindow->isKeyPressed(GLFW_KEY_LEFT_CONTROL)
        ),
        frametime);

    if (flatMovement) {
        glm::vec3 flatForward = glm::normalize(glm::vec3(forward.x, 0.0, forward.z));
        float nextY = moveUp.speed / 2 + position.y < 0.0f ? 0.0f : moveUp.speed / 2;
        position += right * (moveRight.speed / 2) + flatForward * (moveForward.speed / 2) + glm::vec3(0.0f, nextY, 0.0f);
    } else position += right * moveRight.speed + forward * moveForward.speed + up * moveUp.speed;
}