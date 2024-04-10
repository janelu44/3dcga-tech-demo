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

    struct Movement {
        float speed{ 0.0f };
        const float accel{ 0.00003f };
        const float decel{ 0.00005f };
        const float speedCap{ 0.1f };

        void update(int dir, float frametime) {
            if (dir == -1) {
                if (speed > 0.0f)
                    speed -= (accel + decel) * frametime;
                else
                    speed -= accel * frametime;
            }
            else if (dir == 1) {
                if (speed < 0.0f)
                    speed += (accel + decel) * frametime;
                else
                    speed += accel * frametime;
            }
            else {
                if (speed != 0.0f) {
                    float prevSpeed = speed;
                    speed -= decel * frametime * (speed > 0.0f ? 1 : -1);
                    if (prevSpeed * speed < 0.0f)
                        speed = 0.0f;
                }
            }

            speed = (speed < 0.0f ? -1.0f : 1.0f) * std::min(abs(speed), speedCap);
        }
    };

    Movement moveRight, moveForward, moveUp;

private:
    const Window* m_pWindow;
    static constexpr glm::vec3 s_yAxis { 0, 1, 0 };
};


