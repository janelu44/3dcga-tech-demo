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

void Player::updateInput(long long frametime) {
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
            m_pWindow->isKeyPressed(GLFW_KEY_R),
            m_pWindow->isKeyPressed(GLFW_KEY_F)
        ),
        frametime);

    position += right * moveRight.speed + forward * moveForward.speed + up * moveUp.speed;
}