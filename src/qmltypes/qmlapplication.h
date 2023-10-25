/*
 * Copyright (c) 2014-2023 Meltytech, LLC
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

#ifndef QMLAPPLICATION_H
#define QMLAPPLICATION_H

#include <QObject>
#include <QDir>
#include <QPoint>
#include <QColor>
#include <QRect>

namespace Mlt {
class Producer;
}

class QmlApplication : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Qt::WindowModality dialogModality READ dialogModality CONSTANT);
    Q_PROPERTY(QPoint mousePos READ mousePos);
    Q_PROPERTY(QColor toolTipBaseColor READ toolTipBaseColor NOTIFY paletteChanged)
    Q_PROPERTY(QColor toolTipTextColor READ toolTipTextColor NOTIFY paletteChanged)
    Q_PROPERTY(QString OS READ OS CONSTANT)
    Q_PROPERTY(QRect mainWinRect READ mainWinRect);
    Q_PROPERTY(bool hasFiltersOnClipboard READ hasFiltersOnClipboard NOTIFY filtersCopied)
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio CONSTANT)
    Q_PROPERTY(int maxTextureSize READ maxTextureSize CONSTANT)
    Q_PROPERTY(QStringList wipes READ wipes CONSTANT)

public:
    static QmlApplication &singleton();
    static Qt::WindowModality dialogModality();
    static QPoint mousePos();
    static QColor toolTipBaseColor();
    static QColor toolTipTextColor();
    static QString OS();
    static QRect mainWinRect();
    static bool hasFiltersOnClipboard();
    Q_INVOKABLE static void copyFilters();
    Q_INVOKABLE static void pasteFilters();
    Q_INVOKABLE static QString timecode(int frames);
    Q_INVOKABLE static int audioChannels();
    Q_INVOKABLE static QString getNextProjectFile(const QString &filename);
    Q_INVOKABLE static bool isProjectFolder();
    static qreal devicePixelRatio();
    Q_INVOKABLE void showStatusMessage(const QString &message, int timeoutSeconds = 15);
    static int maxTextureSize();
    Q_INVOKABLE static bool confirmOutputFilter();
    static QDir dataDir();
    Q_INVOKABLE static QColor contrastingColor(QString color);
    static QStringList wipes();
    Q_INVOKABLE static bool addWipe(const QString &filePath);
    Q_INVOKABLE static bool intersects(const QRectF &a, const QRectF &b);

signals:
    void paletteChanged();
    void filtersCopied();
    void filtersPasted(Mlt::Producer *);

private:
    explicit QmlApplication();
    QmlApplication(QmlApplication const &);
    void operator=(QmlApplication const &);
};

#endif // QMLAPPLICATION_H
