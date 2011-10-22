/*
 * Copyright (c) 2011 Dan Dennedy <dan@dennedy.org>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <QtGui>
#include <QtOpenGL>
#include <QPalette>
#include "glwidget.h"

#ifndef GL_TEXTURE_RECTANGLE_EXT
#define GL_TEXTURE_RECTANGLE_EXT GL_TEXTURE_RECTANGLE_NV
#endif

GLWidget::GLWidget (QWidget *parent)
    : QGLWidget (parent)
    , m_image_width (0)
    , m_image_height (0)
    , m_texture (0)
    , m_display_ratio (4.0/3.0)
{
    setAttribute (Qt::WA_PaintOnScreen);
    setAttribute (Qt::WA_OpaquePaintEvent);
}

GLWidget::~GLWidget ()
{
    makeCurrent ();
    if (m_texture)
        glDeleteTextures (1, &m_texture);
}

QSize GLWidget::minimumSizeHint () const
{
    return QSize (40, 30);
}

QSize GLWidget::sizeHint () const
{
    return QSize (400, 300);
}

void GLWidget::initializeGL ()
{
    QPalette palette;
    qglClearColor (palette.color (QPalette::Window));
    glShadeModel (GL_FLAT);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
    glDisable (GL_LIGHTING);
    glDisable (GL_DITHER);
    glDisable (GL_BLEND);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
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

    glViewport (0, 0, width, height);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluOrtho2D (0, width, height, 0);
    glMatrixMode (GL_MODELVIEW);
    glClear (GL_COLOR_BUFFER_BIT);
}

void GLWidget::resizeEvent (QResizeEvent* event)
{
    resizeGL (event->size().width(), event->size().height());
}

void GLWidget::paintGL ()
{
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_texture)
    {
#ifdef Q_WS_MAC
        glClear (GL_COLOR_BUFFER_BIT);
#endif
        glEnable (GL_TEXTURE_RECTANGLE_EXT);
        glBegin (GL_QUADS);
            glTexCoord2i (0, 0);
            glVertex2i   (x, y);
            glTexCoord2i (m_image_width - 1, 0);
            glVertex2i   (x + w - 1, y);
            glTexCoord2i (m_image_width - 1, m_image_height - 1);
            glVertex2i   (x + w - 1, y + h - 1);
            glTexCoord2i (0, m_image_height - 1);
            glVertex2i   (x, y + h - 1);
        glEnd ();
        glDisable (GL_TEXTURE_RECTANGLE_EXT);
    }
}

void GLWidget::showImage (QImage image)
{
    m_image_width = image.width ();
    m_image_height = image.height ();

    makeCurrent ();
    if (m_texture)
        glDeleteTextures (1, &m_texture);
    glPixelStorei   (GL_UNPACK_ROW_LENGTH, m_image_width);
    glGenTextures   (1, &m_texture);
    glBindTexture   (GL_TEXTURE_RECTANGLE_EXT, m_texture);
    glTexParameteri (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf (GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D    (GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA8, m_image_width, m_image_height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
    updateGL ();
}
