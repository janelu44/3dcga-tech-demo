#include "planet.h"

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>
DISABLE_WARNINGS_POP()

Planet::Planet(std::filesystem::path meshPath, Shader& shader) {
    m_mesh = GPUMesh::loadMeshGPU(std::move(meshPath));
    m_shader = &shader;
}

void Planet::loadColorMap(std::filesystem::path path) {
    colorTexture = Texture(std::move(path));
}

void Planet::loadNormalMap(std::filesystem::path path) {
    normalTexture = Texture(std::move(path));
}

void Planet::update(glm::mat4 matStart) {
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

    matPosition = glm::mat4(matStart);
    matPosition = glm::rotate(matPosition, glm::radians(orbitAngle.x), glm::vec3(1.0f, 0.0f, 0.0f));
    matPosition = glm::rotate(matPosition, glm::radians(orbitAngle.y), glm::vec3(0.0f, 0.0f, 1.0f));
    matPosition = glm::rotate(matPosition, glm::radians(orbitProgress), glm::vec3(0.0f, 1.0f, 0.0f));
    matPosition = glm::translate(matPosition, glm::vec3(orbitRadius, 0.0f, 0.0f));

    matPosition = glm::rotate(matPosition, glm::radians(-orbitAngle.x), glm::vec3(1.0f, 0.0f, 0.0f));
    matPosition = glm::rotate(matPosition, glm::radians(-orbitAngle.y), glm::vec3(0.0f, 0.0f, 1.0f));
    matPosition = glm::rotate(matPosition, glm::radians(-orbitProgress), glm::vec3(0.0f, 1.0f, 0.0f));

    matRef = glm::rotate(matPosition, glm::radians(orbitProgress), glm::vec3(0.0f, 1.0f, 0.0f));;

    matRotation = glm::rotate(matPosition, glm::radians(revolutionAngle.x), glm::vec3(1.0f, 0.0f, 0.0f));
    matRotation = glm::rotate(matRotation, glm::radians(revolutionAngle.y), glm::vec3(0.0f, 0.0f, 1.0f));
    matRotation = glm::rotate(matRotation, glm::radians(revolutionProgress), glm::vec3(0.0f, 1.0f, 0.0f));

    matScale = glm::scale(matRotation, glm::vec3(radius));
}

glm::mat3 Planet::getNormalMatrix() {
    return glm::inverseTranspose(glm::mat3(matRotation));
}

void Planet::bindShader() {
    m_shader->bind();
}

void Planet::draw() {
    m_mesh[0].draw(Shader());
}


