// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>

class Camera {
public:
    Camera(Window* pWindow, const glm::vec3& position, const glm::vec3& forward);

    void update(bool captureCursor, long long currentTime);
    glm::mat4 viewMatrix() const;
    void rotateX(float angle);
    void rotateY(float angle);
    void setTarget(const glm::vec3& futureForward, const glm::vec3& futureUp, const float timeToEnd);

public:
    glm::vec3 position { 0 };
    glm::vec3 forward { 0, 0, -1 };
    glm::vec3 up { 0, 1, 0 };
    float fov = 70.0f;
    float zNear = 0.01f;
    float zFar = 100.0f;

    bool moveToTarget{ false };
    glm::vec3 initialForward;
    glm::vec3 initialUp;
    glm::vec3 targetForward;
    glm::vec3 targetUp;
    float interpolationSpeed;
    float interpolationProgress;

private:
    static constexpr glm::vec3 s_yAxis { 0, 1, 0 };
    const Window* m_pWindow;
    glm::dvec2 m_prevCursorPos { 0 };
};
