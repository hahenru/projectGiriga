#include <Renderer/UIRenderer.h>
#include <glad/glad.h>
#include <algorithm>

UIRenderer::UIRenderer(Shader& defaultShader, float screenWidth, float screenHeight)
    : m_shader(defaultShader),
      m_screenWidth(screenWidth),
      m_screenHeight(screenHeight) {
    updateProjection();
}

void UIRenderer::setScreenSize(float width, float height) {
    m_screenWidth = width;
    m_screenHeight = height;
    updateProjection();
}

void UIRenderer::begin() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    beginCurrent();
}

void UIRenderer::beginCurrent() {
    m_shader.use();
    m_shader.setMat4(0, m_projection);
}

void UIRenderer::end() {
    while (!m_scissorStack.empty()) {
        popClipRect();
    }
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
}

void UIRenderer::pushClipRect(float x, float y, float width, float height) {
    ScissorRect newRect{ x, y, width, height };

    if (!m_scissorStack.empty()) {
        const ScissorRect& cur = m_scissorStack.back();
        float minX = std::max(cur.x, newRect.x);
        float minY = std::max(cur.y, newRect.y);
        float maxX = std::min(cur.x + cur.width, newRect.x + newRect.width);
        float maxY = std::min(cur.y + cur.height, newRect.y + newRect.height);

        newRect.x = minX;
        newRect.y = minY;
        newRect.width = std::max(0.0f, maxX - minX);
        newRect.height = std::max(0.0f, maxY - minY);
    }

    m_scissorStack.push_back(newRect);
    applyScissor(newRect);
}

void UIRenderer::popClipRect() {
    if (m_scissorStack.empty()) return;
    m_scissorStack.pop_back();

    if (m_scissorStack.empty()) {
        glDisable(GL_SCISSOR_TEST);
    } else {
        applyScissor(m_scissorStack.back());
    }
}

void UIRenderer::drawQuad(float x, float y, float width, float height,
                          float r, float g, float b, float a) {
    glBindTexture(GL_TEXTURE_2D, 0);

    float model[16] = {
        width, 0.0f,   0.0f, 0.0f,
        0.0f,  height, 0.0f, 0.0f,
        0.0f,  0.0f,   1.0f, 0.0f,
        x,     y,      0.0f, 1.0f
    };

    m_shader.setMat4(1, model);
    m_shader.setVec4(2, r, g, b, a);
    m_shader.setInt(3, 0); // ★ 關鍵：isText = false，走純色渲染分支！

    m_quadMesh.draw();
}

void UIRenderer::drawTexturedRect(float x, float y, float width, float height,
                                  unsigned int textureID, const float color[4]) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    float model[16] = {
        width, 0.0f,   0.0f, 0.0f,
        0.0f,  height, 0.0f, 0.0f,
        0.0f,  0.0f,   1.0f, 0.0f,
        x,     y,      0.0f, 1.0f
    };

    m_shader.setMat4(1, model);
    m_shader.setVec4(2, color[0], color[1], color[2], color[3]);
    m_shader.setInt(3, 1); // ★ 關鍵：isText = true，Shader 才會取樣字體單通道 Alpha！

    m_quadMesh.draw();
}

void UIRenderer::updateProjection() {
    float rl = 1.0f / m_screenWidth;
    float tb = -1.0f / m_screenHeight;

    m_projection[0] = 2.0f * rl;  m_projection[4] = 0.0f;       m_projection[8]  = 0.0f;  m_projection[12] = -1.0f;
    m_projection[1] = 0.0f;       m_projection[5] = 2.0f * tb;  m_projection[9]  = 0.0f;  m_projection[13] = 1.0f;
    m_projection[2] = 0.0f;       m_projection[6] = 0.0f;       m_projection[10] = -1.0f; m_projection[14] = 0.0f;
    m_projection[3] = 0.0f;       m_projection[7] = 0.0f;       m_projection[11] = 0.0f;  m_projection[15] = 1.0f;
}

void UIRenderer::applyScissor(const ScissorRect& rect) {
    glEnable(GL_SCISSOR_TEST);
    GLint glX = static_cast<GLint>(rect.x);
    GLint glY = static_cast<GLint>(m_screenHeight - (rect.y + rect.height));
    GLsizei glW = static_cast<GLsizei>(std::max(0.0f, rect.width));
    GLsizei glH = static_cast<GLsizei>(std::max(0.0f, rect.height));
    glScissor(glX, glY, glW, glH);
}