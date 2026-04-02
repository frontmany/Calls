#include "gpuNv12VideoSurface.h"

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <cstring>

namespace
{
    const char* kVs = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
out vec2 vUv;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUv = aUv;
}
)";

    const char* kFs = R"(#version 330 core
in vec2 vUv;
uniform sampler2D texY;
uniform sampler2D texUv;
out vec4 fragColor;
void main() {
    float Y = texture(texY, vUv).r;
    vec2 UV = texture(texUv, vUv).rg - vec2(0.5);
    float R = Y + 1.402 * UV.y;
    float G = Y - 0.344136 * UV.x - 0.714136 * UV.y;
    float B = Y + 1.772 * UV.x;
    fragColor = vec4(clamp(vec3(R, G, B), 0.0, 1.0), 1.0);
}
)";
}

GpuNv12VideoSurface::GpuNv12VideoSurface(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
}

GpuNv12VideoSurface::~GpuNv12VideoSurface()
{
    makeCurrent();
    if (m_texY)
        glDeleteTextures(1, &m_texY);
    if (m_texUv)
        glDeleteTextures(1, &m_texUv);
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    m_program.reset();
    doneCurrent();
}

void GpuNv12VideoSurface::buildShader()
{
    m_program = std::make_unique<QOpenGLShaderProgram>();
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVs);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFs);
    m_program->link();
    m_uTexYLoc = m_program->uniformLocation("texY");
    m_uTexUvLoc = m_program->uniformLocation("texUv");
}

void GpuNv12VideoSurface::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    buildShader();

    const float verts[] = {
        -1.f, -1.f, 0.f, 1.f,
        1.f, -1.f, 1.f, 1.f,
        -1.f, 1.f, 0.f, 0.f,
        1.f, 1.f, 1.f, 0.f,
    };
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glGenTextures(1, &m_texY);
    glGenTextures(1, &m_texUv);
}

void GpuNv12VideoSurface::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void GpuNv12VideoSurface::setNv12Frame(const core::VideoFrameBuffer& frame)
{
    if (frame.format != core::VideoPixelFormat::Nv12 || frame.isEmpty())
    {
        m_hasFrame = false;
        update();
        return;
    }
    m_lastFrame = frame;
    m_hasFrame = true;
    update();
}

void GpuNv12VideoSurface::uploadTextures(const core::VideoFrameBuffer& frame)
{
    const int w = frame.width;
    const int h = frame.height;
    const std::size_t yOff = 0;
    const std::size_t uvOff = frame.uvByteOffset();
    const uint8_t* base = frame.data.data();
    const int strideY = frame.strideY > 0 ? frame.strideY : w;
    const int strideUv = frame.strideUV > 0 ? frame.strideUV : w;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_2D, m_texY);
    if (strideY == w)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, base + yOff);
    }
    else
    {
        m_padY.resize(static_cast<size_t>(w) * static_cast<size_t>(h));
        for (int y = 0; y < h; ++y)
            std::memcpy(m_padY.data() + static_cast<size_t>(y) * w, base + yOff + static_cast<size_t>(y) * strideY, w);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, m_padY.data());
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const int cw = w / 2;
    const int ch = h / 2;
    const uint8_t* uvPtr = base + uvOff;
    glBindTexture(GL_TEXTURE_2D, m_texUv);
    if (strideUv == w)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, cw, ch, 0, GL_RG, GL_UNSIGNED_BYTE, uvPtr);
    }
    else
    {
        m_padUv.resize(static_cast<size_t>(cw) * static_cast<size_t>(ch) * 2);
        for (int y = 0; y < ch; ++y)
            std::memcpy(m_padUv.data() + static_cast<size_t>(y) * cw * 2, uvPtr + static_cast<size_t>(y) * strideUv, cw * 2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, cw, ch, 0, GL_RG, GL_UNSIGNED_BYTE, m_padUv.data());
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void GpuNv12VideoSurface::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_hasFrame || !m_program || m_lastFrame.isEmpty())
        return;

    uploadTextures(m_lastFrame);

    m_program->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texY);
    m_program->setUniformValue(m_uTexYLoc, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texUv);
    m_program->setUniformValue(m_uTexUvLoc, 1);

    const GLint posLoc = m_program->attributeLocation("aPos");
    const GLint uvLoc = m_program->attributeLocation("aUv");
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(uvLoc);
    glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(posLoc);
    glDisableVertexAttribArray(uvLoc);
    m_program->release();
}
