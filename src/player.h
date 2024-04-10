// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>


class Player {
public:
    Player(Window* pWindow, const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up = { 0, 1, 0 });

    void updateInput(long long frametime);

public:
    glm::vec3 position { 0 };
    glm::vec3 forward { 0, 0, -1 };
    glm::vec3 up { 0, 1, 0 };

private:
    const Window* m_pWindow;
    static constexpr glm::vec3 s_yAxis { 0, 1, 0 };
};


