#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()

#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>


DISABLE_WARNINGS_POP()

class EnvMap
{
public:
    EnvMap();

    bool Init(unsigned int WindowWidth, unsigned int WindowHeight);

    void BindForWriting(GLenum CubeFace) const;

    void BindForReading(GLenum TextureUnit) const;

private:
    GLuint m_fbo{0};
    GLuint m_envMap{0};
    GLuint m_depth{0};
};