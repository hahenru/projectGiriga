#include <Renderer/QuadMesh.h>
#include <utility>

QuadMesh::QuadMesh() {
    // 單位 Quad：2 個直角三角形，範圍覆蓋 (0,0) 到 (1,1)
    // 格式：x, y, u, v
    const float vertices[] = {
        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 0.0f,   1.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,

        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,
        0.0f, 1.0f,   0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Attribute 0: 位置座標 vec2
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Attribute 1: 紋理貼圖座標 vec2
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 解除綁定，保持管線狀態乾淨
    glBindVertexArray(0);
}

QuadMesh::~QuadMesh() {
    release();
}

QuadMesh::QuadMesh(QuadMesh&& other) noexcept
: m_vao(other.m_vao), m_vbo(other.m_vbo) {
    other.m_vao = 0;
    other.m_vbo = 0;
}

QuadMesh& QuadMesh::operator=(QuadMesh&& other) noexcept {
    if (this != &other) {
        release();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        other.m_vao = 0;
        other.m_vbo = 0;
    }
    return *this;
}

void QuadMesh::release() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void QuadMesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
