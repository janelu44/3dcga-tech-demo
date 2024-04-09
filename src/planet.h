#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include "mesh.h"
DISABLE_WARNINGS_POP()

class Planet {
public:
    Planet(std::filesystem::path meshPath);

    void update(glm::mat4 matRef);
    glm::mat3 getNormalMatrix();
    void draw();

public:
    float radius = 1.0f;

    float orbitRadius = 2.0f;
    glm::vec3 orbitAngle = glm::vec3(0.0f, 1.0f, 0.0f);
    float orbitSpeed = 1.0f;
    float orbitProgress = 0.0f;

    glm::vec3 revolutionAngle = glm::vec3(0.0f, 1.0f, 0.0f);
    float revolutionSpeed = 1.0f;
    float revolutionProgress = 0.0f;

    glm::mat4 matPosition = glm::mat4(1.0f);
    glm::mat4 matRotation = glm::mat4(1.0f);
    glm::mat4 matScale = glm::mat4(1.0f);

    glm::vec3 baseColor = glm::vec3(1.0f, 0.5f, 0.0f);

private:
    std::vector<GPUMesh> m_mesh;
};
