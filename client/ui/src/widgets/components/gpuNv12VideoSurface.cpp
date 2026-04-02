#include "gpuNv12VideoSurface.h"

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <algorithm>
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
    // PartialUpdate is known to cause GL issues on some platforms when the widget is resized
    // (e.g. window drag / layout changes during screen share).
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
}

void GpuNv12VideoSurface::setKeepAspectRatio(bool keep)
{
    if (m_keepAspectRatio == keep)
        return;
    m_keepAspectRatio = keep;
    update();
}

GpuNv12VideoSurface::~GpuNv12VideoSurface()
{
    if (!isValid())
        return;
    makeCurrent();
    if (!context() || !context()->isValid())
    {
        doneCurrent();
        return;
    }
    if (m_texY)
        glDeleteTextures(1, &m_texY);
    if (m_texUv)
        glDeleteTextures(1, &m_texUv);
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    m_texY = m_texUv = m_vbo = 0;
    m_program.reset();
    doneCurrent();
}

void GpuNv12VideoSurface::buildShader()
{
    m_program = std::make_unique<QOpenGLShaderProgram>();
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVs)
        || !m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFs)
        || !m_program->link())
    {
        m_program.reset();
        return;
    }
    m_uTexYLoc = m_program->uniformLocation("texY");
    m_uTexUvLoc = m_program->uniformLocation("texUv");
}

void GpuNv12VideoSurface::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
    // Make letterbox bars match the Screen background (transparent),
    // so aspect-correct rendering doesn't look like black stripes.
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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
    // Do not call update() here: combined with repeated parent resize it can re-enter
    // paint/resize on some stacks. Qt repaints after resizeGL.
}

void GpuNv12VideoSurface::updateQuadGeometry()
{
    if (!m_vbo || width() <= 0 || height() <= 0 || m_lastFrame.isEmpty())
        return;

    const float rw = static_cast<float>(width());
    const float rh = static_cast<float>(height());
    const float vw = static_cast<float>(m_lastFrame.width);
    const float vh = static_cast<float>(m_lastFrame.height);
    if (vw <= 0.f || vh <= 0.f)
        return;

    float u0 = 0.f;
    float u1 = 1.f;
    float v0 = 0.f;
    float v1 = 1.f;
    float x0 = -1.f;
    float x1 = 1.f;
    float y0 = -1.f;
    float y1 = 1.f;

    if (m_keepAspectRatio)
    {
        const float scale = std::min(rw / vw, rh / vh);
        const float dw = vw * scale;
        const float dh = vh * scale;
        const float ox = (rw - dw) * 0.5f;
        const float oy = (rh - dh) * 0.5f;
        x0 = 2.f * ox / rw - 1.f;
        x1 = 2.f * (ox + dw) / rw - 1.f;
        const float yBottom = rh - oy - dh;
        const float yTop = rh - oy;
        y0 = 2.f * yBottom / rh - 1.f;
        y1 = 2.f * yTop / rh - 1.f;
    }
    else
    {
        const float s = std::max(rw / vw, rh / vh);
        const float sx = rw / (vw * s);
        const float sy = rh / (vh * s);
        if (sx < 1.f - 1e-6f)
        {
            u0 = (1.f - sx) * 0.5f;
            u1 = 1.f - u0;
        }
        if (sy < 1.f - 1e-6f)
        {
            v0 = (1.f - sy) * 0.5f;
            v1 = 1.f - v0;
        }
    }

    const float verts[] = {
        x0, y0, u0, v1,
        x1, y0, u1, v1,
        x0, y1, u0, v0,
        x1, y1, u1, v0,
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
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
    if (!m_texY || !m_texUv)
        return;
    const int w = frame.width;
    const int h = frame.height;
    if (w <= 0 || h <= 0)
        return;
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
    if (width() <= 0 || height() <= 0)
        return;
    if (!isValid())
        return;

    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_hasFrame || !m_program || !m_program->isLinked() || m_lastFrame.isEmpty())
        return;
    if (!m_texY || !m_texUv || !m_vbo)
        return;

    uploadTextures(m_lastFrame);
    updateQuadGeometry();

    m_program->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texY);
    m_program->setUniformValue(m_uTexYLoc, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texUv);
    m_program->setUniformValue(m_uTexUvLoc, 1);

    const GLint posLoc = m_program->attributeLocation("aPos");
    const GLint uvLoc = m_program->attributeLocation("aUv");
    if (posLoc < 0 || uvLoc < 0)
    {
        m_program->release();
        return;
    }
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
