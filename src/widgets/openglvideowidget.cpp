/*
 * Copyright (c) 2011-2023 Meltytech, LLC
 *
 * Some GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

#include "openglvideowidget.h"
#include <Logger.h>
#include <QOpenGLVersionFunctionsFactory>
#include <QOpenGLFunctions_1_1>
#include <QOpenGLFunctions_3_2_Core>
#include <utility>

#ifdef QT_NO_DEBUG
#define check_error(fn) {}
#else
#define check_error(fn) { int err = fn->glGetError(); if (err != GL_NO_ERROR) { LOG_ERROR() << "GL error"  << Qt::hex << err << Qt::dec << "at" << __FILE__ << ":" << __LINE__; } }
#endif

OpenGLVideoWidget::OpenGLVideoWidget(QObject *parent)
    : VideoWidget{parent}
    , m_quickContext(nullptr)
    , m_isThreadedOpenGL(false)
{
    m_renderTexture[0] = m_renderTexture[1] = m_renderTexture[2] = 0;
    m_displayTexture[0] = m_displayTexture[1] = m_displayTexture[2] = 0;
}

OpenGLVideoWidget::~OpenGLVideoWidget()
{
    LOG_DEBUG() << "begin";
    if (m_renderTexture[0] && m_displayTexture[0] && m_context) {
        m_context->makeCurrent(&m_offscreenSurface);
        m_context->functions()->glDeleteTextures(3, m_renderTexture);
        if (m_displayTexture[0] && m_displayTexture[1] && m_displayTexture[2])
            m_context->functions()->glDeleteTextures(3, m_displayTexture);
        m_context->doneCurrent();
    }
}

void OpenGLVideoWidget::initialize()
{
    LOG_DEBUG() << "begin";
    auto context = static_cast<QOpenGLContext *>(quickWindow()->rendererInterface()->getResource(
                                                     quickWindow(), QSGRendererInterface::OpenGLContextResource));
    m_quickContext = context;

    if (!m_offscreenSurface.isValid()) {
        m_offscreenSurface.setFormat(context->format());
        m_offscreenSurface.create();
    }
    Q_ASSERT(m_offscreenSurface.isValid());

    initializeOpenGLFunctions();
    LOG_INFO() << "OpenGL vendor" << QString::fromUtf8((const char *) glGetString(GL_VENDOR));
    LOG_INFO() << "OpenGL renderer" << QString::fromUtf8((const char *) glGetString(GL_RENDERER));
    LOG_INFO() << "OpenGL threaded?" << context->supportsThreadedOpenGL();
    LOG_INFO() << "OpenGL ES?" << context->isOpenGLES();
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);
    LOG_INFO() << "OpenGL maximum texture size =" << m_maxTextureSize;
    GLint dims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &dims[0]);
    LOG_INFO() << "OpenGL maximum viewport size =" << dims[0] << "x" << dims[1];

    createShader();

    LOG_DEBUG() << "end";
    Mlt::VideoWidget::initialize();
}

void OpenGLVideoWidget::createShader()
{
    m_shader.reset(new QOpenGLShaderProgram);
    m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                      "uniform highp mat4 projection;"
                                      "uniform highp mat4 modelView;"
                                      "attribute highp vec4 vertex;"
                                      "attribute highp vec2 texCoord;"
                                      "varying highp vec2 coordinates;"
                                      "void main(void) {"
                                      "  gl_Position = projection * modelView * vertex;"
                                      "  coordinates = texCoord;"
                                      "}");
    m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                      "uniform sampler2D Ytex, Utex, Vtex;"
                                      "uniform lowp int colorspace;"
                                      "varying highp vec2 coordinates;"
                                      "void main(void) {"
                                      "  mediump vec3 texel;"
                                      "  texel.r = texture2D(Ytex, coordinates).r -  16.0/255.0;" // Y
                                      "  texel.g = texture2D(Utex, coordinates).r - 128.0/255.0;" // U
                                      "  texel.b = texture2D(Vtex, coordinates).r - 128.0/255.0;" // V
                                      "  mediump mat3 coefficients;"
                                      "  if (colorspace == 601) {"
                                      "    coefficients = mat3("
                                      "      1.1643,  1.1643,  1.1643," // column 1
                                      "      0.0,    -0.39173, 2.017," // column 2
                                      "      1.5958, -0.8129,  0.0);" // column 3
                                      "  } else {" // ITU-R 709
                                      "    coefficients = mat3("
                                      "      1.1643, 1.1643, 1.1643," // column 1
                                      "      0.0,   -0.213,  2.112," // column 2
                                      "      1.793, -0.533,  0.0);" // column 3
                                      "  }"
                                      "  gl_FragColor = vec4(coefficients * texel, 1.0);"
                                      "}");
    m_shader->link();
    m_textureLocation[0] = m_shader->uniformLocation("Ytex");
    m_textureLocation[1] = m_shader->uniformLocation("Utex");
    m_textureLocation[2] = m_shader->uniformLocation("Vtex");
    m_colorspaceLocation = m_shader->uniformLocation("colorspace");
    m_projectionLocation = m_shader->uniformLocation("projection");
    m_modelViewLocation = m_shader->uniformLocation("modelView");
    m_vertexLocation = m_shader->attributeLocation("vertex");
    m_texCoordLocation = m_shader->attributeLocation("texCoord");
}

static void uploadTextures(QOpenGLContext *context, const SharedFrame &frame, GLuint texture[])
{
    int width = frame.get_image_width();
    int height = frame.get_image_height();
    const uint8_t *image = frame.get_image(mlt_image_yuv420p);
    QOpenGLFunctions *f = context->functions();

    // The planes of pixel data may not be a multiple of the default 4 bytes.
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Upload each plane of YUV to a texture.
    if (texture[0])
        f->glDeleteTextures(3, texture);
    check_error(f);
    f->glGenTextures(3, texture);
    check_error(f);

    f->glBindTexture  (GL_TEXTURE_2D, texture[0]);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0,
                       GL_LUMINANCE, GL_UNSIGNED_BYTE, image);
    check_error(f);

    f->glBindTexture  (GL_TEXTURE_2D, texture[1]);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0,
                       GL_LUMINANCE, GL_UNSIGNED_BYTE, image + width * height);
    check_error(f);

    f->glBindTexture  (GL_TEXTURE_2D, texture[2]);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    check_error(f);
    f->glTexImage2D   (GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0,
                       GL_LUMINANCE, GL_UNSIGNED_BYTE, image + width * height + width / 2 * height / 2);
    check_error(f);
}

void OpenGLVideoWidget::renderVideo()
{
    auto context = static_cast<QOpenGLContext *>(quickWindow()->rendererInterface()->getResource(
                                                     quickWindow(), QSGRendererInterface::OpenGLContextResource));
#ifndef QT_NO_DEBUG
    QOpenGLFunctions *f = context->functions();
#endif
    float width = this->width() * devicePixelRatioF();
    float height = this->height() * devicePixelRatioF();

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glViewport(0, 0, width, height);
    check_error(f);

    if (!m_isThreadedOpenGL) {
        m_mutex.lock();
        if (!m_sharedFrame.is_valid()) {
            m_mutex.unlock();
            return;
        }
        uploadTextures(context, m_sharedFrame, m_displayTexture);
        m_mutex.unlock();
    }

    if (!m_displayTexture[0]) {
        return;
    }

    quickWindow()->beginExternalCommands();

    // Bind textures.
    for (int i = 0; i < 3; ++i) {
        if (m_displayTexture[i]) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_displayTexture[i]);
            check_error(f);
        }
    }

    // Init shader program.
    m_shader->bind();
    m_shader->setUniformValue(m_textureLocation[0], 0);
    m_shader->setUniformValue(m_textureLocation[1], 1);
    m_shader->setUniformValue(m_textureLocation[2], 2);
    m_shader->setUniformValue(m_colorspaceLocation, MLT.profile().colorspace());
    check_error(f);

    // Setup an orthographic projection.
    QMatrix4x4 projection;
    projection.scale(2.0f / width, 2.0f / height);
    m_shader->setUniformValue(m_projectionLocation, projection);
    check_error(f);

    // Set model view.
    QMatrix4x4 modelView;
    if (rect().width() > 0.0 && zoom() > 0.0) {
        if (offset().x() || offset().y())
            modelView.translate(-offset().x() * devicePixelRatioF(),
                                offset().y() * devicePixelRatioF());
        modelView.scale(zoom(), zoom());
    }
    m_shader->setUniformValue(m_modelViewLocation, modelView);
    check_error(f);

    // Provide vertices of triangle strip.
    QVector<QVector2D> vertices;
    width = rect().width() * devicePixelRatioF();
    height = rect().height() * devicePixelRatioF();
    vertices << QVector2D(-width / 2.0f, -height / 2.0f);
    vertices << QVector2D(-width / 2.0f, height / 2.0f);
    vertices << QVector2D(width / 2.0f, -height / 2.0f);
    vertices << QVector2D(width / 2.0f, height / 2.0f);
    m_shader->enableAttributeArray(m_vertexLocation);
    check_error(f);
    m_shader->setAttributeArray(m_vertexLocation, vertices.constData());
    check_error(f);

    // Provide texture coordinates.
    QVector<QVector2D> texCoord;
    texCoord << QVector2D(0.0f, 1.0f);
    texCoord << QVector2D(0.0f, 0.0f);
    texCoord << QVector2D(1.0f, 1.0f);
    texCoord << QVector2D(1.0f, 0.0f);
    m_shader->enableAttributeArray(m_texCoordLocation);
    check_error(f);
    m_shader->setAttributeArray(m_texCoordLocation, texCoord.constData());
    check_error(f);

    // Render
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());
    check_error(f);

    // Cleanup
    m_shader->disableAttributeArray(m_vertexLocation);
    m_shader->disableAttributeArray(m_texCoordLocation);
    m_shader->release();
    for (int i = 0; i < 3; ++i) {
        if (m_displayTexture[i]) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
            check_error(f);
        }
    }
    glActiveTexture(GL_TEXTURE0);
    check_error(f);

    quickWindow()->endExternalCommands();
    Mlt::VideoWidget::renderVideo();
}

void OpenGLVideoWidget::onFrameDisplayed(const SharedFrame &frame)
{
    if (m_isThreadedOpenGL && !m_context) {
        m_context.reset(new QOpenGLContext);
        if (m_context) {
            m_context->setFormat(m_quickContext->format());
            m_context->setShareContext(m_quickContext);
            m_context->create();
        }
    }
    if (m_context && m_context->isValid()) {
        // Using threaded OpenGL to upload textures.
        QOpenGLFunctions *f = m_context->functions();
        m_context->makeCurrent(&m_offscreenSurface);
        uploadTextures(m_context.get(), frame, m_renderTexture);
        f->glBindTexture(GL_TEXTURE_2D, 0);
        check_error(f);
        f->glFinish();
        m_context->doneCurrent();

        m_mutex.lock();
        for (int i = 0; i < 3; ++i)
            std::swap(m_renderTexture[i], m_displayTexture[i]);
        m_mutex.unlock();
    }
    Mlt::VideoWidget::onFrameDisplayed(frame);
}

