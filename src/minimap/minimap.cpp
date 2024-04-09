#include "minimap.h"

#include <framework/disable_all_warnings.h>

DISABLE_WARNINGS_PUSH()

#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <vector>

DISABLE_WARNINGS_POP()

void Minimap::Init(int width, int height) {
    // Overide the default resolution
    m_resolution = {width, height};

    // Create the framebuffer
    glGenFramebuffers(1, &m_fbo);

    // Create the depth buffer
    glGenTextures(1, &m_depth);
    glBindTexture(GL_TEXTURE_2D, m_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Create the color buffer
    glGenTextures(1, &m_minimap);
    glBindTexture(GL_TEXTURE_2D, m_minimap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    // Attach the depth buffer to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth, 0);

    // Disable reads and writes to the color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create a quad vao for rendering the minimap
    std::vector<glm::vec3> quadVertices{
            {-1.0f, -1.0f, -1.0f},
            {1.0f,  -1.0f, -1.0f},
            {1.0f,  1.0f,  -1.0f},
            {-1.0f, 1.0f,  -1.0f},
    };

    std::vector<glm::uvec3> quadTriangles{
            {0, 1, 2},
            {2, 3, 0},
    };

    GLuint vbo;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, static_cast<GLsizeiptr>(quadVertices.size() * sizeof(glm::uvec3)),quadVertices.data(), 0);

    GLuint ibo;
    glCreateBuffers(1, &ibo);
    glNamedBufferStorage(ibo, static_cast<GLsizeiptr>(quadTriangles.size() * sizeof(glm::uvec3)),quadTriangles.data(), 0);

    glCreateVertexArrays(1, &m_vao);
    glVertexArrayElementBuffer(m_vao, ibo);

    glVertexArrayVertexBuffer(m_vao, 0, vbo, 0, sizeof(glm::vec3));
    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);
}

void Minimap::Draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(3 * 2), GL_UNSIGNED_INT, nullptr);
}

void Minimap::BindForWriting() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_minimap, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void Minimap::BindForReading(GLenum TextureUnit) const {
    glActiveTexture(TextureUnit);
    glBindTexture(GL_TEXTURE_2D, m_minimap);
}

