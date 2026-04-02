#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <memory>
#include <vector>

#include "videoFrameBuffer.h"

/**
 * Renders NV12 using two GL textures (Y + UV) and a YUV->RGB shader.
 * RGB frames are not handled here; use CPU/Screen path for local RGB preview.
 */
class GpuNv12VideoSurface final : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GpuNv12VideoSurface(QWidget* parent = nullptr);
    ~GpuNv12VideoSurface() override;

    void setNv12Frame(const core::VideoFrameBuffer& frame);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void buildShader();
    void uploadTextures(const core::VideoFrameBuffer& frame);

    std::unique_ptr<QOpenGLShaderProgram> m_program;
    GLuint m_vbo = 0;
    GLuint m_texY = 0;
    GLuint m_texUv = 0;
    int m_uMvpLoc = -1;
    int m_uTexYLoc = -1;
    int m_uTexUvLoc = -1;

    bool m_hasFrame = false;
    core::VideoFrameBuffer m_lastFrame;
    std::vector<uint8_t> m_padY;
    std::vector<uint8_t> m_padUv;
};
