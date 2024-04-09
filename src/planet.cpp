#include "planet.h"

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
DISABLE_WARNINGS_POP()

Planet::Planet(std::filesystem::path meshPath) {
    m_mesh = GPUMesh::loadMeshGPU(std::move(meshPath));
}

void Planet::update(glm::mat4 matRef) {
    // UPDATE ORBIT
    orbitProgress += orbitSpeed;
    if (orbitProgress <= 0.0f) {
        orbitProgress += 360.0f;
    }
    if (orbitProgress >= 360.0f) {
        orbitProgress -= 360.0f;
    }

    // UPDATE REVOLUTION
    revolutionProgress += revolutionSpeed;
    if (revolutionProgress <= 0.0f) {
        revolutionProgress += 360.0f;
    }
    if (revolutionProgress >= 360.0f) {
        revolutionProgress -= 360.0f;
    }

    matPosition = glm::mat4(matRef);
    matPosition = glm::rotate(matPosition, glm::radians(orbitProgress), orbitAngle);
    matPosition = glm::translate(matPosition, glm::vec3(orbitRadius, 0.0f, 0.0f));

    matRotation = glm::rotate(matPosition, glm::radians(revolutionProgress - orbitProgress), revolutionAngle);
    matScale = glm::scale(matRotation, glm::vec3(radius));
}

glm::mat3 Planet::getNormalMatrix() {
    return glm::inverseTranspose(glm::mat3(matRotation));
}

void Planet::draw() {
    m_mesh[0].draw(Shader());
}


