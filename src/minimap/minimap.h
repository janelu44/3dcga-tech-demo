#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>

class Minimap {
public:
    Minimap() = default;
    void Init(int width, int height);
    void BindForWriting() const;
    void BindForReading(GLenum TextureUnit) const;
    void Draw() const;

public:
    float m_distance{7.0f};
    glm::ivec2 m_resolution{526};

private:
    GLuint m_fbo{0};
    GLuint m_depth{0};
    GLuint m_minimap{0};
    GLuint m_vao{0};
};
