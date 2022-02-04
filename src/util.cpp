/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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

#include "util.h"

#include <QFileInfo>
#include <QWidget>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMap>
#include <QDoubleSpinBox>
#include <QTemporaryFile>
#include <QApplication>
#include <QCryptographicHash>
#include <QtGlobal>
#include <QCameraInfo>

#include <MltChain.h>
#include <MltProducer.h>
#include <Logger.h>
#include "shotcut_mlt_properties.h"
#include "qmltypes/qmlapplication.h"
#include "proxymanager.h"
#include <memory>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_MAC
static const unsigned int kLowMemoryThresholdPercent = 10U;
#else
static const unsigned int kLowMemoryThresholdKB = 256U * 1024U;
#endif

QString Util::baseName(const QString &filePath, bool trimQuery)
{
    QString s = filePath;
    // Only if absolute path and not a URI.
    if (s.startsWith('/') || s.midRef(1, 2) == ":/" || s.midRef(1, 2) == ":\\")
        s = QFileInfo(s).fileName();
    if (trimQuery) {
        return removeQueryString(s);
    }
    return s;
}

void Util::setColorsToHighlight(QWidget* widget, QPalette::ColorRole role)
{
    if (role == QPalette::Base) {
        widget->setStyleSheet(
                    "QLineEdit {"
                        "font-weight: bold;"
                        "background-color: palette(highlight);"
                        "color: palette(highlighted-text);"
                        "selection-background-color: palette(alternate-base);"
                        "selection-color: palette(text);"
                    "}"
                    "QLineEdit:hover {"
                        "border: 2px solid palette(button-text);"
                    "}"
        );
    } else {
        QPalette palette = QApplication::palette();
        palette.setColor(role, palette.color(palette.Highlight));
        palette.setColor(role == QPalette::Button ? QPalette::ButtonText : QPalette::WindowText,
            palette.color(palette.HighlightedText));
        widget->setPalette(palette);
        widget->setAutoFillBackground(true);
    }
}

void Util::showInFolder(const QString& path)
{
    QFileInfo info(removeQueryString(path));
#if defined(Q_OS_WIN)
    QStringList args;
    if (!info.isDir())
        args << "/select,";
    args << QDir::toNativeSeparators(path);
    if (QProcess::startDetached("explorer", args))
        return;
#elif defined(Q_OS_MAC)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \"" + path + "\"";
    args << "-e";
    args << "end tell";
#if !defined(QT_DEBUG)
    args << "-e";
    args << "return";
#endif
    if (!QProcess::execute("/usr/bin/osascript", args))
        return;
#endif
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir()? path : info.path()));
}

bool Util::warnIfNotWritable(const QString& filePath, QWidget* parent, const QString& caption)
{
    // Returns true if not writable.
    if (!filePath.isEmpty() && !filePath.contains("://")) {
        QFileInfo info(filePath);
        if (!info.isDir()) {
            info = QFileInfo(info.dir().path());
        }
        if (!info.isWritable()) {
            info = QFileInfo(filePath);
            QMessageBox::warning(parent, caption,
                                 QObject::tr("Unable to write file %1\n"
                                    "Perhaps you do not have permission.\n"
                                    "Try again with a different folder.")
                                 .arg(info.fileName()));
            return true;
        }
    }
    return false;
}

QString Util::producerTitle(const Mlt::Producer& producer)
{
    QString result;
    Mlt::Producer& p = const_cast<Mlt::Producer&>(producer);
    if (!p.is_valid() || p.is_blank()) return result;
    if (p.get(kShotcutTransitionProperty))
        return QObject::tr("Transition");
    if (p.get(kTrackNameProperty))
        return QObject::tr("Track: %1").arg(QString::fromUtf8(p.get(kTrackNameProperty)));
    if (mlt_service_tractor_type == p.type())
        return QObject::tr("Output");
    if (p.get(kShotcutCaptionProperty))
        return QString::fromUtf8(p.get(kShotcutCaptionProperty));
    return Util::baseName(ProxyManager::resource(p));
}

QString Util::removeFileScheme(QUrl& url)
{
    QString path = url.url();
    if (url.scheme() == "file")
        path = url.toString(QUrl::PreferLocalFile);
    return QUrl::fromPercentEncoding(path.toUtf8());
}

static inline bool isValidGoProFirstFilePrefix(const QFileInfo& info)
{
    QStringList list {"GOPR", "GH01", "GL01", "GM01", "GS01", "GX01"};
    return list.contains(info.baseName().left(4).toUpper());
}

static inline bool isValidGoProPrefix(const QFileInfo& info)
{
    QStringList list {"GP", "GH", "GL", "GM", "GS", "GX"};
    return list.contains(info.baseName().left(2).toUpper());
}

static inline bool isValidGoProSuffix(const QFileInfo& info)
{
    QStringList list {"MP4", "LRV", "360", "WAV"};
    return list.contains(info.suffix().toUpper());
}

const QStringList Util::sortedFileList(const QList<QUrl>& urls)
{
    QStringList result;
    QMap<QString, QStringList> goproFiles;

    // First look for GoPro main files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && isValidGoProSuffix(fi) && isValidGoProFirstFilePrefix(fi)) {
            goproFiles[fi.baseName().mid(4)] << fi.filePath();
        }
    }
    // Then, look for GoPro split files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && isValidGoProSuffix(fi) && isValidGoProPrefix(fi) && !isValidGoProFirstFilePrefix(fi)) {
            QString goproNumber = fi.baseName().mid(4);
            // Only if there is a matching main GoPro file.
            if (goproFiles.contains(goproNumber) && goproFiles[goproNumber].size()) {
                goproFiles[goproNumber] << fi.filePath();
            }
        }
    }
    // Next, sort the GoPro files.
    foreach (QString goproNumber, goproFiles.keys())
        goproFiles[goproNumber].sort(Qt::CaseSensitive);
    // Finally, build the list of all files.
    // Add all the GoPro files first.
    foreach (QStringList paths, goproFiles)
        result << paths;
    // Add all the non-GoPro files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url));
        if (fi.baseName().size() == 8 && isValidGoProSuffix(fi) &&
                (isValidGoProFirstFilePrefix(fi) || isValidGoProPrefix(fi))) {
            QString goproNumber = fi.baseName().mid(4);
            if (goproFiles.contains(goproNumber) && goproFiles[goproNumber].contains(fi.filePath()))
                continue;
        }
        result << fi.filePath();
    }
    return result;
}

int Util::coerceMultiple(int value, int multiple)
{
    return (value + multiple - 1) / multiple * multiple;
}

QList<QUrl> Util::expandDirectories(const QList<QUrl>& urls)
{
    QList<QUrl> result;
    foreach (QUrl url, urls) {
        QString path = Util::removeFileScheme(url);
        QFileInfo fi(path);
        if (fi.isDir()) {
            QDir dir(path);
            foreach (QFileInfo fi, dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name))
                result << fi.filePath();
        } else {
            result << url;
        }
    }
    return result;
}

bool Util::isDecimalPoint(QChar ch)
{
    // See https://en.wikipedia.org/wiki/Decimal_separator#Unicode_characters
    return ch == '.' || ch == ',' || ch == '\'' || ch == ' '
        || ch == QChar(0x00B7) || ch == QChar(0x2009) || ch == QChar(0x202F)
        || ch == QChar(0x02D9) || ch == QChar(0x066B) || ch == QChar(0x066C)
        || ch == QChar(0x2396);
}

bool Util::isNumeric(QString& str)
{
    for (int i = 0; i < str.size(); ++i) {
        QCharRef ch = str[i];
        if (ch != '+' && ch != '-' && ch.toLower() != 'e'
            && !isDecimalPoint(ch) && !ch.isDigit())
            return false;
    }
    return true;
}

bool Util::convertNumericString(QString& str, QChar decimalPoint)
{
    // Returns true if the string was changed.
    bool result = false;
    if (isNumeric(str)) {
        for (int i = 0; i < str.size(); ++i) {
            QCharRef ch = str[i];
            if (ch != decimalPoint && isDecimalPoint(ch)) {
                ch = decimalPoint;
                result = true;
            }
        }
    }
    return result;
}

bool Util::convertDecimalPoints(QString& str, QChar decimalPoint)
{
    // Returns true if the string was changed.
    bool result = false;
    if (!str.contains(decimalPoint)) {
        for (int i = 0; i < str.size(); ++i) {
            QCharRef ch = str[i];
            // Space is used as a delimiter for rect fields and possibly elsewhere.
            if (ch != decimalPoint && ch != ' ' && isDecimalPoint(ch)) {
                ch = decimalPoint;
                result = true;
            }
        }
    }
    return result;
}

void Util::showFrameRateDialog(const QString& caption, int numerator, QDoubleSpinBox* spinner, QWidget *parent)
{
    double fps = numerator / 1001.0;
    QMessageBox dialog(QMessageBox::Question, caption,
                       QObject::tr("The value you entered is very similar to the common,\n"
                          "more standard %1 = %2/1001.\n\n"
                          "Do you want to use %1 = %2/1001 instead?")
                          .arg(fps, 0, 'f', 6).arg(numerator),
                       QMessageBox::No | QMessageBox::Yes,
                       parent);
    dialog.setDefaultButton(QMessageBox::Yes);
    dialog.setEscapeButton(QMessageBox::No);
    dialog.setWindowModality(QmlApplication::dialogModality());
    if (dialog.exec() == QMessageBox::Yes) {
        spinner->setValue(fps);
    }
}

QTemporaryFile* Util::writableTemporaryFile(const QString& filePath, const QString& templateName)
{
    // filePath should already be checked writable.
    QFileInfo info(filePath);
    QString templateFileName = templateName.isEmpty()?
        QString("%1.XXXXXX").arg(QCoreApplication::applicationName()) : templateName;

    // First, try the system temp dir.
    QString templateFilePath = QDir(QDir::tempPath()).filePath(templateFileName);
    QScopedPointer<QTemporaryFile> tmp(new QTemporaryFile(templateFilePath));

    if (!tmp->open() || tmp->write("") < 0) {
        // Otherwise, use the directory provided.
        return new QTemporaryFile(info.dir().filePath(templateFileName));
    } else {
        return tmp.take();
    }
}

void Util::applyCustomProperties(Mlt::Producer& destination, Mlt::Producer& source, int in, int out)
{
    Mlt::Properties p(destination);
    p.clear("force_progressive");
    p.clear("force_tff");
    p.clear("force_aspect_ratio");
    p.clear("video_delay");
    p.clear("color_range");
    p.clear("speed");
    p.clear("warp_speed");
    p.clear("warp_pitch");
    p.clear("rotate");
    p.clear(kAspectRatioNumerator);
    p.clear(kAspectRatioDenominator);
    p.clear(kCommentProperty);
    p.clear(kShotcutProducerProperty);
    p.clear(kDefaultAudioIndexProperty);
    p.clear(kOriginalInProperty);
    p.clear(kOriginalOutProperty);
    if (!p.get_int(kIsProxyProperty))
        p.clear(kOriginalResourceProperty);
    destination.pass_list(source, "mlt_service, audio_index, video_index, force_progressive, force_tff,"
                       "force_aspect_ratio, video_delay, color_range, warp_speed, warp_pitch, rotate,"
                       kAspectRatioNumerator ","
                       kAspectRatioDenominator ","
                       kCommentProperty ","
                       kShotcutProducerProperty ","
                       kDefaultAudioIndexProperty ","
                       kOriginalInProperty ","
                       kOriginalOutProperty ","
                       kOriginalResourceProperty ","
                       kDisableProxyProperty);
    if (!destination.get("_shotcut:resource")) {
        destination.set("_shotcut:resource", destination.get("resource"));
        destination.set("_shotcut:length", destination.get("length"));
    }
    QString resource = ProxyManager::resource(destination);
    if (!qstrcmp("timewarp", source.get("mlt_service"))) {
        auto speed = qAbs(source.get_double("warp_speed"));
        auto caption = QString("%1 (%2x)").arg(Util::baseName(resource, true)).arg(speed);
        destination.set(kShotcutCaptionProperty, caption.toUtf8().constData());

        resource = destination.get("_shotcut:resource");
        destination.set("warp_resource", resource.toUtf8().constData());
        resource = QString("%1:%2:%3").arg("timewarp", source.get("warp_speed"), resource);
        destination.set("resource", resource.toUtf8().constData());
        double speedRatio = 1.0 / speed;
        int length = qRound(destination.get_length() * speedRatio);
        destination.set("length", destination.frames_to_time(length, mlt_time_clock));
    } else {
        auto caption = Util::baseName(resource, true);
        destination.set(kShotcutCaptionProperty, caption.toUtf8().constData());

        p.clear("warp_resource");
        destination.set("resource", destination.get("_shotcut:resource"));
        destination.set("length", destination.get("_shotcut:length"));
    }
    destination.set_in_and_out(in, out);
}

QString Util::getFileHash(const QString& path)
{
    // This routine is intentionally copied from Kdenlive.
    QFile file(removeQueryString(path));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData;
         // 1 MB = 1 second per 450 files (or faster)
         // 10 MB = 9 seconds per 450 files (or faster)
        if (file.size() > 1000000*2) {
            fileData = file.read(1000000);
            if (file.seek(file.size() - 1000000))
                fileData.append(file.readAll());
        } else {
            fileData = file.readAll();
        }
        file.close();
        return QCryptographicHash::hash(fileData, QCryptographicHash::Md5).toHex();
    }
    return QString();
}

QString Util::getHash(Mlt::Properties& properties)
{
    QString hash = properties.get(kShotcutHashProperty);
    if (hash.isEmpty()) {
        QString service = properties.get("mlt_service");
        QString resource = QString::fromUtf8(properties.get("resource"));

        if (properties.get_int(kIsProxyProperty) && properties.get(kOriginalResourceProperty))
            resource = QString::fromUtf8(properties.get(kOriginalResourceProperty));
        else if (service == "timewarp")
            resource = QString::fromUtf8(properties.get("warp_resource"));
        else if (service == "vidstab")
            resource = QString::fromUtf8(properties.get("filename"));
        hash = getFileHash(resource);
        if (!hash.isEmpty())
            properties.set(kShotcutHashProperty, hash.toLatin1().constData());
    }
    return hash;
}

bool Util::hasDriveLetter(const QString& path)
{
    auto driveSeparators = path.midRef(1, 2);
    return driveSeparators == ":/" || driveSeparators == ":\\";
}

QFileDialog::Options Util::getFileDialogOptions()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (qEnvironmentVariableIsSet("SNAP")) {
        return QFileDialog::DontUseNativeDialog;
    }
#endif
    return QFileDialog::Options();
}

bool Util::isMemoryLow()
{
#if defined(Q_OS_WIN)
    unsigned int availableKB = UINT_MAX;
    MEMORYSTATUSEX memory_status;
    ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
    memory_status.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memory_status)) {
        availableKB = memory_status.ullAvailPhys / 1024UL;
    }
    LOG_INFO() << "available RAM = " << availableKB << "KB";
    return availableKB < kLowMemoryThresholdKB;
#elif defined(Q_OS_MAC)
    QProcess p;
    p.start("memory_pressure");
    p.waitForFinished();
    auto lines = p.readAllStandardOutput();
    p.close();
    for (const auto& line : lines.split('\n')) {
        if (line.startsWith("System-wide memory free")) {
            auto fields = line.split(':');
            for (auto s : fields) {
                bool ok = false;
                auto percentage = s.replace('%', "").toUInt(&ok);
                if (ok) {
                    LOG_INFO() << percentage << '%';
                    return percentage <= kLowMemoryThresholdPercent;
                }
            }
        }
    }
    return false;
#elif defined(__FreeBSD__)
    QProcess p;
    p.start("sysctl -n hw.usermem");
    p.waitForFinished();
    auto lines = p.readAllStandardOutput();
    p.close();
    bool ok = false;
    auto availableKB = lines.toUInt(&ok);
    if (ok) {
	    return availableKB < kLowMemoryThresholdKB;
    }

    return false;
#elif defined(Q_OS_LINUX)
    unsigned int availableKB = UINT_MAX;
    QFile meminfo("/proc/meminfo");
    if (meminfo.open(QIODevice::ReadOnly)) {
        for (auto line = meminfo.readLine(1024); availableKB == UINT_MAX && !line.isEmpty(); line = meminfo.readLine(1024)) {
            if (line.startsWith("MemAvailable")) {
                const auto& fields = line.split(' ');
                for (const auto& s : fields) {
                    bool ok = false;
                    auto kB = s.toUInt(&ok);
                    if (ok) {
                        availableKB = kB;
                        break;
                    }
                }
            }
        }
    }
    meminfo.close();
    LOG_INFO() << "available RAM = " << availableKB << "KB";
    return availableKB < kLowMemoryThresholdKB;
#endif
}

QString Util::removeQueryString(const QString& s)
{
    auto i = s.lastIndexOf("\\?");
    if (i < 0) {
        i = s.lastIndexOf("%5C?");
    }
    if (i > 0 ) {
        return s.left(i);
    }
    return s;
}

int Util::greatestCommonDivisor(int m, int n)
{
    int gcd, remainder;
    while (n) {
        remainder = m % n;
        m = n;
        n = remainder;
    }
    gcd = m;
    return gcd;
}

void Util::normalizeFrameRate(double fps, int& numerator, int& denominator)
{
    // Convert some common non-integer frame rates to fractions.
    if (qRound(fps * 1000000.0) == 23976024) {
        numerator = 24000;
        denominator = 1001;
    } else if (qRound(fps * 100000.0) == 2997003) {
        numerator = 30000;
        denominator = 1001;
    } else if (qRound(fps * 1000000.0) == 47952048) {
        numerator = 48000;
         denominator = 1001;
    } else if (qRound(fps * 100000.0) == 5994006) {
        numerator = 60000;
        denominator = 1001;
    } else {
        // Workaround storing QDoubleSpinBox::value() loses precision.
        numerator = qRound(fps * 1000000.0);
        denominator = 1000000;
    }
}

QString Util::textColor(const QColor& color)
{
    return (color.value() < 150)? "white" : "black";
}

void Util::cameraFrameRateSize(const QByteArray &deviceName, qreal& frameRate, QSize& size)
{
    std::unique_ptr<QCamera> camera(new QCamera(deviceName));
    if (camera) {
        camera->load();
        QCameraViewfinderSettings viewfinderSettings;
        auto resolutions = camera->supportedViewfinderResolutions(viewfinderSettings);
        if (resolutions.size() > 0) {
            LOG_INFO() << "resolutions:" << resolutions;
            // Get the highest resolution
            viewfinderSettings.setResolution(resolutions.first());
            for (auto& resolution : resolutions) {
                if (resolution.width() > viewfinderSettings.resolution().width() && resolution.height() > viewfinderSettings.resolution().height()) {
                    viewfinderSettings.setResolution(resolution);
                }
            }
            auto frameRates = camera->supportedViewfinderFrameRateRanges(viewfinderSettings);
            if (frameRates.size() > 0) {
                // Get the highest frame rate for the chosen resolution
                viewfinderSettings.setMaximumFrameRate(frameRates.first().maximumFrameRate);
                for (auto& frameRate : frameRates) {
                    LOG_INFO() << "frame rate:" << frameRate.maximumFrameRate;
                    if (frameRate.maximumFrameRate > viewfinderSettings.maximumFrameRate()) {
                        viewfinderSettings.setMaximumFrameRate(frameRate.maximumFrameRate);
                    }
                }
            }
        }
        camera->unload();
        frameRate = viewfinderSettings.maximumFrameRate();
        size = viewfinderSettings.resolution();
    }
}
