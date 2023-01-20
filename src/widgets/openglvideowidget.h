/*
 * Copyright (c) 2023 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENGLVIDEOWIDGET_H
#define OPENGLVIDEOWIDGET_H

#include "videowidget.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOffscreenSurface>

class OpenGLVideoWidget : public Mlt::VideoWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit OpenGLVideoWidget(QObject *parent = nullptr);
    virtual ~OpenGLVideoWidget();

public slots:
    virtual void initialize();
    virtual void renderVideo();
    virtual void onFrameDisplayed(const SharedFrame &frame);

private:
    void createShader();

    QOffscreenSurface m_offscreenSurface;
    std::unique_ptr<QOpenGLShaderProgram> m_shader;
    GLint m_projectionLocation;
    GLint m_modelViewLocation;
    GLint m_vertexLocation;
    GLint m_texCoordLocation;
    GLint m_colorspaceLocation;
    GLint m_textureLocation[3];
    QOpenGLContext *m_quickContext;
    std::unique_ptr<QOpenGLContext> m_context;
    GLuint m_renderTexture[3];
    GLuint m_displayTexture[3];
};

#endif // OPENGLVIDEOWIDGET_H
