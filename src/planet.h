#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()

#include <glm/mat4x4.hpp>
#include "mesh.h"
#include "texture.h"

DISABLE_WARNINGS_POP()

class Planet {
public:
    Planet(std::filesystem::path meshPath, Shader &shader);

    void loadColorMap(std::filesystem::path path);

    void loadNormalMap(std::filesystem::path path);

    void update(glm::mat4 matStart);

    glm::mat3 getNormalMatrix();

    void bindShader();

    void draw();

public:
    float radius = 1.0f;

    float orbitRadius = 2.0f;
    glm::vec2 orbitAngle = glm::vec2(0.0f);
    float orbitSpeed = 1.0f;
    float orbitProgress = 0.0f;

    glm::vec2 revolutionAngle = glm::vec2(0.0f);
    float revolutionSpeed = 1.0f;
    float revolutionProgress = 0.0f;

    glm::mat4 matPosition = glm::mat4(1.0f);
    glm::mat4 matRotation = glm::mat4(1.0f);
    glm::mat4 matScale = glm::mat4(1.0f);
    glm::mat4 matRef = glm::mat4(1.0f);

    glm::vec3 baseColor = glm::vec3(1.0f, 0.5f, 0.0f);

    std::optional<Texture> colorTexture;
    std::optional<Texture> normalTexture;

private:
    std::vector<GPUMesh> m_mesh;
    Shader *m_shader;
};