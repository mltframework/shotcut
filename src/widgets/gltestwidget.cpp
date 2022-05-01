/*
 * Copyright (c) 2013-2016 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "gltestwidget.h"
#include <Logger.h>
#include <QMessageBox>
#include <QCoreApplication>
#include <Logger.h>
#include "settings.h"

GLTestWidget::GLTestWidget(QWidget *parent) :
    QGLWidget(parent),
    m_isInitialized(false)
{
    hide();
    LOG_INFO() << "OpenGL context version" << format().majorVersion() << format().minorVersion();
}

void GLTestWidget::initializeGL()
{
    if (!m_isInitialized) {
        m_isInitialized = true;
        initializeOpenGLFunctions();

        bool supported = (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_0);
        supported = supported && hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);
        supported = supported && hasOpenGLFeature(QOpenGLFunctions::Shaders);
        supported = supported && hasOpenGLFeature(QOpenGLFunctions::Framebuffers);

        if (!supported) {
            LOG_WARNING() << "OpenGL has NPOTTextures" << hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);
            LOG_WARNING() << "OpenGL has Shaders" << hasOpenGLFeature(QOpenGLFunctions::Shaders);
            LOG_WARNING() << "OpenGL has Framebuffers" << hasOpenGLFeature(QOpenGLFunctions::Framebuffers);
            QMessageBox::critical(this, qApp->applicationName(),
                                  tr("Error:\nThis program requires OpenGL version 2.0\nwith the framebuffer object extension."));
            ::exit(EXIT_FAILURE);
        }
        deleteLater();
    }
}
