/*
 * Copyright (c) 2020-2022 Meltytech, LLC
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

#include "qimagejob.h"
#include "util.h"

#include <QImage>
#include <QImageReader>
#include <QRunnable>
#include <QtConcurrent/QtConcurrent>

QImageJob::QImageJob(const QString &destFilePath, const QString &srcFilePath, const int height)
    : AbstractJob(srcFilePath)
    , m_srcFilePath(srcFilePath)
    , m_destFilePath(destFilePath)
    , m_height(height)
{
    setTarget(destFilePath);
    setLabel(tr("Make proxy for %1").arg(Util::baseName(srcFilePath)));
}

QImageJob::~QImageJob()
{
    if (m_destFilePath.contains("proxies") && m_destFilePath.contains(".pending.")) {
        QFile::remove(m_destFilePath);
    }
}

void QImageJob::start()
{
    AbstractJob::start();
    auto result = QtConcurrent::run([ = ]() {
        appendToLog(QStringLiteral("Reading source image \"%1\"\n").arg(m_srcFilePath));
        QImageReader reader;
        reader.setAutoTransform(true);
        reader.setDecideFormatFromContent(true);
        reader.setFileName(m_srcFilePath);
        QImage image(reader.read());
        if (!image.isNull()) {
            image = image.scaledToHeight(m_height, Qt::SmoothTransformation);
            if (image.save(m_destFilePath)) {
                appendToLog(QStringLiteral("Successfully saved image as \"%1\"\n").arg(m_destFilePath));
                QMetaObject::invokeMethod(this, "onFinished", Qt::QueuedConnection, Q_ARG(int, 0));
            } else {
                appendToLog(QStringLiteral("Failed to save image as \"%1\"\n").arg(m_destFilePath));
                QMetaObject::invokeMethod(this, "onFinished", Qt::QueuedConnection, Q_ARG(int, 1));
            }
        } else {
            appendToLog(QStringLiteral("Failed to read source image \"%1\"\n").arg(m_srcFilePath));
            QMetaObject::invokeMethod(this, "onFinished", Qt::QueuedConnection, Q_ARG(int, 1));
        }
    });
}
