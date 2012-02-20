/*
 * Copyright (c) 2011 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
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

#include "GLee/GLee.h"
#include <QtGui>
#include <QtOpenGL>
#include <QPalette>
#include <Mlt.h>
#include "glwidget.h"

#ifndef GL_TEXTURE_RECTANGLE_EXT
#define GL_TEXTURE_RECTANGLE_EXT GL_TEXTURE_RECTANGLE_NV
#endif

using namespace Mlt;

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent)
    , Controller()
    , showFrameSemaphore(3)
    , m_image_width(0)
    , m_image_height(0)
    , m_display_ratio(4.0/3.0)
{
    m_texture[0] = m_texture[1] = m_texture[2] = 0;
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    if (m_texture[0])
        glDeleteTextures(3, m_texture);
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(40, 30);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 300);
}

void GLWidget::initializeGL()
{
    QPalette palette;
    qglClearColor(palette.color(QPalette::Window));
    glShadeModel(GL_FLAT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    m_shader.addShaderFromSourceCode(QGLShader::Fragment,
        "#extension GL_ARB_texture_rectangle : enable\n"
        "uniform sampler2DRect Ytex, Utex, Vtex;"
        "void main(void) {"
        "  float r, g, b;"
        "  vec4 txl, ux, vx;"
        "  float nx = gl_TexCoord[0].x;"
        "  float ny = gl_TexCoord[0].y;"
        "  float y = texture2DRect(Ytex, vec2(nx, ny)).r;"
        "  float u = texture2DRect(Utex, vec2(nx/2.0, ny/4.0)).r;"
        "  float v = texture2DRect(Vtex, vec2(nx/2.0, ny/4.0)).r;"

        "  y = 1.1643 * (y - 0.0625);"
        "  u = u - 0.5;"
        "  v = v - 0.5;"

        "  r = y + 1.5958  * v;"
        "  g = y - 0.39173 * u - 0.81290 * v;"
        "  b = y + 2.017   * u;"

        "  gl_FragColor = vec4(r, g, b, 1.0);"
        "}");
    m_shader.bind();
}

void GLWidget::resizeGL(int width, int height)
{
    double this_aspect = (double) width / height;

    // Special case optimisation to negate odd effect of sample aspect ratio
    // not corresponding exactly with image resolution.
    if ((int) (this_aspect * 1000) == (int) (m_display_ratio * 1000))
    {
        w = width;
        h = height;
    }
    // Use OpenGL to normalise sample aspect ratio
    else if (height * m_display_ratio > width)
    {
        w = width;
        h = width / m_display_ratio;
    }
    else
    {
        w = height * m_display_ratio;
        h = height;
    }
    x = (width - w) / 2;
    y = (height - h) / 2;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0);
    glMatrixMode(GL_MODELVIEW);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLWidget::resizeEvent(QResizeEvent* event)
{
    resizeGL(event->size().width(), event->size().height());
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_texture[0])
    {
#ifdef Q_WS_MAC
        glClear(GL_COLOR_BUFFER_BIT);
#endif
        glEnable(GL_TEXTURE_RECTANGLE_EXT);
        glBegin(GL_QUADS);
            glTexCoord2i(0, 0);
            glVertex2i  (x, y);
            glTexCoord2i(m_image_width, 0);
            glVertex2i  (x + w, y);
            glTexCoord2i(m_image_width, m_image_height);
            glVertex2i  (x + w, y + h);
            glTexCoord2i(0, m_image_height);
            glVertex2i  (x, y + h);
        glEnd();
        glDisable(GL_TEXTURE_RECTANGLE_EXT);
    }
}

void GLWidget::showFrame(Mlt::QFrame frame)
{
    m_image_width = 0;
    m_image_height = 0;
    mlt_image_format format = mlt_image_yuv420p;
    const uint8_t* image = frame.frame()->get_image(format, m_image_width, m_image_height);

    // Copy each plane of YUV to a texture bound to shader program˙.
    makeCurrent();
    if (m_texture[0])
        glDeleteTextures(3, m_texture);
    glPixelStorei  (GL_UNPACK_ROW_LENGTH, m_image_width);
    glGenTextures  (3, m_texture);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture  (GL_TEXTURE_RECTANGLE_EXT, m_texture[0]);
    m_shader.setUniformValue(m_shader.uniformLocation("Ytex"), 0);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D   (GL_TEXTURE_RECTANGLE_EXT, 0, GL_LUMINANCE, m_image_width, m_image_height, 0,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, image);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture  (GL_TEXTURE_RECTANGLE_EXT, m_texture[1]);
    m_shader.setUniformValue(m_shader.uniformLocation("Utex"), 1);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D   (GL_TEXTURE_RECTANGLE_EXT, 0, GL_LUMINANCE, m_image_width/2, m_image_height/4, 0,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, image + m_image_width * m_image_height);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture  (GL_TEXTURE_RECTANGLE_EXT, m_texture[2]);
    m_shader.setUniformValue(m_shader.uniformLocation("Vtex"), 2);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D   (GL_TEXTURE_RECTANGLE_EXT, 0, GL_LUMINANCE, m_image_width/2, m_image_height/4, 0,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, image + m_image_width * m_image_height + m_image_width/2 * m_image_height/2);

    glDraw();
    showFrameSemaphore.release();
}

int GLWidget::open(Mlt::Producer* producer)
{
    int error = Controller::open(producer);

    if (!error) {
        // use SDL for audio, OpenGL for video
        m_consumer = new Mlt::Consumer(profile(), "sdl_audio");
        if (m_consumer->is_valid()) {
            // Connect the producer to the consumer - tell it to "run" later
            m_consumer->connect(*m_producer);
            // Make an event handler for when a frame's image should be displayed
            m_consumer->listen("consumer-frame-show", this, (mlt_listener) on_frame_show);
            connect(this, SIGNAL(frameReceived(Mlt::QFrame, int)),
                    this, SLOT(showFrame(Mlt::QFrame)), Qt::UniqueConnection);
            m_consumer->set("progressive", property("progressive").toBool());
            m_consumer->set("rescale", property("rescale").toString().toAscii().constData());
            m_consumer->set("deinterlace_method", property("deinterlace_method").toString().toAscii().constData());
            m_display_ratio = profile().dar();
            resizeGL(width(), height());
        }
        else {
            // Cleanup on error
            error = 2;
            Controller::close();
        }
    }
    return error;
}

// MLT consumer-frame-show event handler
void GLWidget::on_frame_show(mlt_consumer, void* self, mlt_frame frame_ptr)
{
    GLWidget* widget = static_cast<GLWidget*>(self);
    if (widget->showFrameSemaphore.tryAcquire()) {
        Frame frame(frame_ptr);
        emit widget->frameReceived(Mlt::QFrame(frame), frame.get_position());
    }
}
