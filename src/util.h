/*
 * Copyright (c) 2014-2018 Meltytech, LLC
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

#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QPalette>
#include <QUrl>

class QWidget;
namespace Mlt {
    class Producer;
}

class Util
{
private:
    Util() {}
public:
    static QString baseName(const QString &filePath);
    static void setColorsToHighlight(QWidget* widget, QPalette::ColorRole role = QPalette::Window);
    static void showInFolder(const QString &path);
    static bool warnIfNotWritable(const QString& filePath, QWidget* parent, const QString& caption);
    static QString producerTitle(const Mlt::Producer& producer);
    static QString removeFileScheme(QUrl &url);
    static QStringList sortedFileList(const QList<QUrl>& urls);
    static int coerceMultiple(int value, int multiple = 2);
    static QList<QUrl> expandDirectories(const QList<QUrl>& urls);
    static uint versionStringToUInt(const QString& s);
};

#endif // UTIL_H
