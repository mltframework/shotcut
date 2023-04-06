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

#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QPalette>
#include <QUrl>
#include <QFileDialog>
#include <MltProperties.h>

class QWidget;
class QDoubleSpinBox;
class QTemporaryFile;

namespace Mlt {
class Producer;
}

class Util
{
private:
    Util() {}
public:
    static QString baseName(const QString &filePath, bool trimQuery = false);
    static void setColorsToHighlight(QWidget *widget, QPalette::ColorRole role = QPalette::Window);
    static void showInFolder(const QString &path);
    static bool warnIfNotWritable(const QString &filePath, QWidget *parent, const QString &caption);
    static QString producerTitle(const Mlt::Producer &producer);
    static QString removeFileScheme(QUrl &url, bool fromPercentEncoding = true);
    static const QStringList sortedFileList(const QList<QUrl> &urls);
    static int coerceMultiple(int value, int multiple = 2);
    static QList<QUrl> expandDirectories(const QList<QUrl> &urls);
    static bool isDecimalPoint(QChar ch);
    static bool isNumeric(QString &str);
    static bool convertNumericString(QString &str, QChar decimalPoint);
    static bool convertDecimalPoints(QString &str, QChar decimalPoint);
    static void showFrameRateDialog(const QString &caption, int numerator, QDoubleSpinBox *spinner,
                                    QWidget *parent = Q_NULLPTR);
    static QTemporaryFile *writableTemporaryFile(const QString &filePath = QString(),
                                                 const QString &templateName = QString());
    static void applyCustomProperties(Mlt::Producer &destination, Mlt::Producer &source, int in,
                                      int out);
    static QString getFileHash(const QString &path);
    static QString getHash(Mlt::Properties &properties);
    static bool hasDriveLetter(const QString &path);
    static QFileDialog::Options getFileDialogOptions();
    static bool isMemoryLow();
    static QString removeQueryString(const QString &s);
    static int greatestCommonDivisor(int m, int n);
    static void normalizeFrameRate(double fps, int &numerator, int &denominator);
    static QString textColor(const QColor &color);
    static void cameraFrameRateSize(const QByteArray &deviceName, qreal &frameRate, QSize &size);
    static bool ProducerIsTimewarp(Mlt::Producer *producer);
    static QString GetFilenameFromProducer(Mlt::Producer *producer, bool useOriginal = true);
    static double GetSpeedFromProducer(Mlt::Producer *producer);
    static QString updateCaption(Mlt::Producer *producer);
    static void passProducerProperties(Mlt::Producer *src, Mlt::Producer *dst);
    static bool warnIfLowDiskSpace(const QString &path);
};

#endif // UTIL_H
