/*
 * Copyright (c) 2014-2024 Meltytech, LLC
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
#include <QMediaDevices>
#include <QCamera>
#include <QCameraDevice>
#include <QStorageInfo>
#include <QCheckBox>

#include <MltChain.h>
#include <MltProducer.h>
#include <Logger.h>
#include "dialogs/transcodedialog.h"
#include "mainwindow.h"
#include "shotcut_mlt_properties.h"
#include "qmltypes/qmlapplication.h"
#include "proxymanager.h"
#include "settings.h"
#include "transcoder.h"

#include <math.h>
#include <memory>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_MAC
static const unsigned int kLowMemoryThresholdPercent = 10U;
#else
static const unsigned int kLowMemoryThresholdKB = 256U * 1024U;
#endif
static const qint64 kFreeSpaceThesholdGB = 25LL * 1024 * 1024 * 1024;

QString Util::baseName(const QString &filePath, bool trimQuery)
{
    QString s = filePath;
    // Only if absolute path and not a URI.
    if (s.startsWith('/') || s.mid(1, 2) == ":/" || s.mid(1, 2) == ":\\")
        s = QFileInfo(s).fileName();
    if (trimQuery) {
        return removeQueryString(s);
    }
    return s;
}

void Util::setColorsToHighlight(QWidget *widget, QPalette::ColorRole role)
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

void Util::showInFolder(const QString &path)
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
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.isDir() ? path : info.path()));
}

bool Util::warnIfNotWritable(const QString &filePath, QWidget *parent, const QString &caption)
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

QString Util::producerTitle(const Mlt::Producer &producer)
{
    QString result;
    Mlt::Producer &p = const_cast<Mlt::Producer &>(producer);
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

QString Util::removeFileScheme(QUrl &url, bool fromPercentEncoding)
{
    QString path = url.url();
    if (url.scheme() == "file")
        path = url.toString(QUrl::PreferLocalFile);
    if (fromPercentEncoding)
        return QUrl::fromPercentEncoding(path.toUtf8());
    return path;
}

static inline bool isValidGoProFirstFilePrefix(const QFileInfo &info)
{
    QStringList list {"GOPR", "GH01", "GL01", "GM01", "GS01", "GX01"};
    return list.contains(info.baseName().left(4).toUpper());
}

static inline bool isValidGoProPrefix(const QFileInfo &info)
{
    QStringList list {"GP", "GH", "GL", "GM", "GS", "GX"};
    return list.contains(info.baseName().left(2).toUpper());
}

static inline bool isValidGoProSuffix(const QFileInfo &info)
{
    QStringList list {"MP4", "LRV", "360", "WAV"};
    return list.contains(info.suffix().toUpper());
}

const QStringList Util::sortedFileList(const QList<QUrl> &urls)
{
    QStringList result;
    QMap<QString, QStringList> goproFiles;

    // First look for GoPro main files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url, false));
        if (fi.baseName().size() == 8 && isValidGoProSuffix(fi) && isValidGoProFirstFilePrefix(fi)) {
            goproFiles[fi.baseName().mid(4)] << fi.filePath();
        }
    }
    // Then, look for GoPro split files.
    foreach (QUrl url, urls) {
        QFileInfo fi(removeFileScheme(url, false));
        if (fi.baseName().size() == 8 && isValidGoProSuffix(fi) && isValidGoProPrefix(fi)
                && !isValidGoProFirstFilePrefix(fi)) {
            QString goproNumber = fi.baseName().mid(4);
            // Only if there is a matching main GoPro file.
            if (goproFiles.contains(goproNumber) && goproFiles[goproNumber].size()) {
                goproFiles[goproNumber] << fi.filePath();
            }
        }
    }
    // Next, sort the GoPro files.
    auto keys = goproFiles.keys();
    for (auto &goproNumber : keys) {
        goproFiles[goproNumber].sort(Qt::CaseSensitive);
    }
    // Finally, build the list of all files.
    // Add all the GoPro files first.
    for (auto &paths : goproFiles) {
        result << paths;
    }
    // Add all the non-GoPro files.
    for (auto url : urls) {
        QFileInfo fi(removeFileScheme(url, false));
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

QList<QUrl> Util::expandDirectories(const QList<QUrl> &urls)
{
    QList<QUrl> result;
    foreach (QUrl url, urls) {
        QString path = Util::removeFileScheme(url, false);
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

bool Util::isNumeric(QString &str)
{
    for (int i = 0; i < str.size(); ++i) {
        auto ch = str[i];
        if (ch != '+' && ch != '-' && ch.toLower() != 'e'
                && !isDecimalPoint(ch) && !ch.isDigit())
            return false;
    }
    return true;
}

bool Util::convertNumericString(QString &str, QChar decimalPoint)
{
    // Returns true if the string was changed.
    bool result = false;
    if (isNumeric(str)) {
        for (int i = 0; i < str.size(); ++i) {
            auto ch = str[i];
            if (ch != decimalPoint && isDecimalPoint(ch)) {
                ch = decimalPoint;
                result = true;
            }
        }
    }
    return result;
}

bool Util::convertDecimalPoints(QString &str, QChar decimalPoint)
{
    // Returns true if the string was changed.
    bool result = false;
    if (!str.contains(decimalPoint)) {
        for (int i = 0; i < str.size(); ++i) {
            auto ch = str[i];
            // Space is used as a delimiter for rect fields and possibly elsewhere.
            if (ch != decimalPoint && ch != ' ' && isDecimalPoint(ch)) {
                ch = decimalPoint;
                result = true;
            }
        }
    }
    return result;
}

void Util::showFrameRateDialog(const QString &caption, int numerator, QDoubleSpinBox *spinner,
                               QWidget *parent)
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

QTemporaryFile *Util::writableTemporaryFile(const QString &filePath, const QString &templateName)
{
    // filePath should already be checked writable.
    QFileInfo info(filePath);
    QString templateFileName = templateName.isEmpty() ?
                               QStringLiteral("%1.XXXXXX").arg(QCoreApplication::applicationName()) : templateName;

    // First, try the system temp dir.
    QString templateFilePath = QDir(QDir::tempPath()).filePath(templateFileName);
    std::unique_ptr<QTemporaryFile> tmp(new QTemporaryFile(templateFilePath));

    if (!tmp->open() || tmp->write("") < 0) {
        // Otherwise, use the directory provided.
        return new QTemporaryFile(info.dir().filePath(templateFileName));
    } else {
        return tmp.release();
    }
}

void Util::applyCustomProperties(Mlt::Producer &destination, Mlt::Producer &source, int in, int out)
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
    destination.pass_list(source,
                          "mlt_service, audio_index, video_index, astream, vstream, force_progressive, force_tff,"
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
        auto caption = QStringLiteral("%1 (%2x)").arg(Util::baseName(resource, true)).arg(speed);
        destination.set(kShotcutCaptionProperty, caption.toUtf8().constData());

        resource = destination.get("_shotcut:resource");
        destination.set("warp_resource", resource.toUtf8().constData());
        resource = QStringLiteral("%1:%2:%3").arg("timewarp", source.get("warp_speed"), resource);
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

QString Util::getFileHash(const QString &path)
{
    // This routine is intentionally copied from Kdenlive.
    QFile file(removeQueryString(path));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData;
        // 1 MB = 1 second per 450 files (or faster)
        // 10 MB = 9 seconds per 450 files (or faster)
        if (file.size() > 1000000 * 2) {
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

QString Util::getHash(Mlt::Properties &properties)
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

bool Util::hasDriveLetter(const QString &path)
{
    auto driveSeparators = path.mid(1, 2);
    return driveSeparators == ":/" || driveSeparators == ":\\";
}

QFileDialog::Options Util::getFileDialogOptions()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (qEnvironmentVariableIsSet("SNAP") || qEnvironmentVariableIsSet("GNOME_SHELL_SESSION_MODE")) {
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
    p.start("memory_pressure", QStringList());
    p.waitForFinished();
    auto lines = p.readAllStandardOutput();
    p.close();
    for (auto &line : lines.split('\n')) {
        if (line.startsWith("System-wide memory free")) {
            const auto fields = line.split(':');
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
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
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
        for (auto line = meminfo.readLine(1024); availableKB == UINT_MAX
                && !line.isEmpty(); line = meminfo.readLine(1024)) {
            if (line.startsWith("MemAvailable")) {
                const auto &fields = line.split(' ');
                for (const auto &s : fields) {
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

QString Util::removeQueryString(const QString &s)
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

void Util::normalizeFrameRate(double fps, int &numerator, int &denominator)
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
        auto gcd = greatestCommonDivisor(numerator, denominator);
        numerator /= gcd;
        denominator /= gcd;
    }
}

QString Util::textColor(const QColor &color)
{
    return (color.value() < 150) ? "white" : "black";
}

void Util::cameraFrameRateSize(const QByteArray &deviceName, qreal &frameRate, QSize &size)
{
    std::unique_ptr<QCamera> camera;
    for (const QCameraDevice &cameraDevice : QMediaDevices::videoInputs()) {
        if (cameraDevice.id() == deviceName) {
            camera.reset(new QCamera(cameraDevice));
            break;
        }
    }
    if (camera) {
        auto currentFormat = camera->cameraDevice().videoFormats().first();
        QList<QSize> resolutions;
        for (const auto &format : camera->cameraDevice().videoFormats()) {
            resolutions << format.resolution();
        }
        if (resolutions.size() > 0) {
            LOG_INFO() << "resolutions:" << resolutions;
            // Get the highest resolution
            camera->setCameraFormat(currentFormat);
            for (const auto &format : camera->cameraDevice().videoFormats()) {
                if (format.resolution().width() > currentFormat.resolution().width()
                        && format.resolution().height() > currentFormat.resolution().height()) {
                    camera->setCameraFormat(format);
                    currentFormat = format;
                }
            }
        }
        if (currentFormat.maxFrameRate() > 0) {
            frameRate = currentFormat.maxFrameRate();
        }
        if (currentFormat.resolution().width() > 0) {
            size = currentFormat.resolution();
        }
    }
}

bool Util::ProducerIsTimewarp(Mlt::Producer *producer)
{
    return QString::fromUtf8(producer->get("mlt_service")) == "timewarp";
}

QString Util::GetFilenameFromProducer(Mlt::Producer *producer, bool useOriginal)
{
    QString resource;
    if (useOriginal && producer->get(kOriginalResourceProperty)) {
        resource = QString::fromUtf8(producer->get(kOriginalResourceProperty));
    } else if (ProducerIsTimewarp(producer)) {
        resource = QString::fromUtf8(producer->get("resource"));
        auto i = resource.indexOf(':');
        if (producer->get_int(kIsProxyProperty) && i > 0) {
            resource = resource.mid(i + 1);
        } else {
            resource = QString::fromUtf8(producer->get("warp_resource"));
        }
    } else {
        resource = QString::fromUtf8(producer->get("resource"));
    }
    if (QFileInfo(resource).isRelative()) {
        QString basePath = QFileInfo(MAIN.fileName()).canonicalPath();
        QFileInfo fi(basePath, resource);
        resource = fi.filePath();
    }
    return resource;
}

double Util::GetSpeedFromProducer(Mlt::Producer *producer)
{
    double speed = 1.0;
    if (ProducerIsTimewarp(producer)) {
        speed = fabs(producer->get_double("warp_speed"));
    }
    return speed;
}

QString Util::updateCaption(Mlt::Producer *producer)
{
    double warpSpeed = GetSpeedFromProducer(producer);
    QString resource = GetFilenameFromProducer(producer);
    QString name = Util::baseName(resource, true);
    QString caption = producer->get(kShotcutCaptionProperty);
    if (caption.isEmpty() || caption.startsWith(name)) {
        // compute the caption
        if (warpSpeed != 1.0)
            caption = QStringLiteral("%1 (%2x)").arg(name).arg(warpSpeed);
        else
            caption = name;
        producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
    }
    return caption;
}

void Util::passProducerProperties(Mlt::Producer *src, Mlt::Producer *dst)
{
    dst->pass_list(*src, "audio_index, video_index, astream, vstream, force_aspect_ratio,"
                   "video_delay, force_progressive, force_tff, force_full_range, color_range, warp_pitch, rotate,"
                   kAspectRatioNumerator ","
                   kAspectRatioDenominator ","
                   kShotcutHashProperty ","
                   kPlaylistIndexProperty ","
                   kShotcutSkipConvertProperty ","
                   kCommentProperty ","
                   kDefaultAudioIndexProperty ","
                   kShotcutCaptionProperty ","
                   kOriginalResourceProperty ","
                   kDisableProxyProperty ","
                   kIsProxyProperty ","
                   kShotcutProducerProperty);
    QString shotcutProducer(src->get(kShotcutProducerProperty));
    QString service(src->get("mlt_service"));
    if (service.startsWith("avformat") || shotcutProducer == "avformat")
        dst->set(kShotcutProducerProperty, "avformat");
}

bool Util::warnIfLowDiskSpace(const QString &path)
{
    // Check if the drive this file will be on is getting low on space.
    if (Settings.encodeFreeSpaceCheck()) {
        QStorageInfo si(QFileInfo(path).path());
        LOG_DEBUG() << si.bytesAvailable() << "bytes available on" << si.displayName();
        if (si.isValid() && si.bytesAvailable() < kFreeSpaceThesholdGB) {
            QMessageBox dialog(QMessageBox::Question, QApplication::applicationDisplayName(),
                               QObject::tr("The drive you chose only has %1 MiB of free space.\n"
                                           "Do you still want to continue?")
                               .arg(si.bytesAvailable() / 1024 / 1024),
                               QMessageBox::No | QMessageBox::Yes);
            dialog.setWindowModality(QmlApplication::dialogModality());
            dialog.setDefaultButton(QMessageBox::Yes);
            dialog.setEscapeButton(QMessageBox::No);
            dialog.setCheckBox(new QCheckBox(QObject::tr("Do not show this anymore.",
                                                         "Export free disk space warning dialog")));
            int result = dialog.exec();
            if (dialog.checkBox()->isChecked())
                Settings.setEncodeFreeSpaceCheck(false);
            if (result == QMessageBox::No) {
                return true;
            }
        }
    }
    return false;
}

bool Util::isFpsDifferent(double a, double b)
{
    return qAbs(a - b) > 0.001;
}

QString Util::getNextFile(const QString &filePath)
{
    QFileInfo info(filePath);
    QString basename = info.completeBaseName();
    QString extension = info.suffix();
    if (extension.isEmpty()) {
        extension = basename;
        basename = QString();
    }
    for (unsigned i = 1; i < std::numeric_limits<unsigned>::max(); i++) {
        QString filename = QString::fromLatin1("%1%2.%3").arg(basename).arg(i).arg(extension);
        if (!info.dir().exists(filename))
            return info.dir().filePath(filename);
    }
    return filePath;
}

QString Util::trcString(int trc)
{
    QString trcString = QObject::tr("unknown (%1)").arg(trc);
    switch (trc) {
    case 0:
        trcString = QObject::tr("NA");
        break;
    case 1:
        trcString = "ITU-R BT.709";
        break;
    case 6:
        trcString = "ITU-R BT.601";
        break;
    case 7:
        trcString = "SMPTE ST240";
        break;
    case 11:
        trcString = "IEC 61966-2-4";
        break;
    case 13:
        trcString = "sRGB";
        break;
    case 14:
        trcString = "ITU-R BT.2020";
        break;
    case 15:
        trcString = "ITU-R BT.2020";
        break;
    case 16:
        trcString = "SMPTE ST2084 (PQ)";
        break;
    case 17:
        trcString = "SMPTE ST428";
        break;
    case 18:
        trcString = "ARIB B67 (HLG)";
        break;
    }
    return trcString;
}

bool Util::trcIsCompatible(int trc)
{
    // Transfer characteristics > SMPTE240M Probably need conversion except IEC61966-2-4 is OK
    return trc <= 7 || trc == 11 || trc == 13 || trc == 18;
}

QString Util::getConversionAdvice(Mlt::Producer *producer)
{
    QString advice;
    producer->probe();
    QString resource = Util::GetFilenameFromProducer(producer);
    int trc = producer->get_int("meta.media.color_trc");
    if (!Util::trcIsCompatible(trc)) {
        QString trcString = Util::trcString(trc);
        LOG_INFO() << resource << "Probable HDR" << trcString;
        advice = QObject::tr("This file uses color transfer characteristics %1, which may result in incorrect colors or brightness in Shotcut.").arg(
                     trcString);
    } else if (producer->get_int("meta.media.variable_frame_rate")) {
        LOG_INFO() << resource << "is variable frame rate";
        advice = QObject::tr("This file is variable frame rate, which is not reliable for editing.");
    } else if (QFile::exists(resource) && !MLT.isSeekable(producer)) {
        LOG_INFO() << resource << "is not seekable";
        advice = QObject::tr("This file does not support seeking and cannot be used for editing.");
    } else if (QFile::exists(resource) && resource.endsWith(".m2t")) {
        LOG_INFO() << resource << "is HDV";
        advice = QObject::tr("This file format (HDV) is not reliable for editing.");
    }
    return advice;
}

mlt_color Util::mltColorFromQColor(const QColor &color)
{
    return mlt_color {
        static_cast<uint8_t>(color.red()),
        static_cast<uint8_t>(color.green()),
        static_cast<uint8_t>(color.blue()),
        static_cast<uint8_t>(color.alpha())
    };
}

void Util::offerSingleFileConversion(QString &message, Mlt::Producer *producer, QWidget *parent)
{
    TranscodeDialog dialog(message.append(
                               QObject::tr(" Do you want to convert it to an edit-friendly format?\n\n"
                                           "If yes, choose a format below and then click OK to choose a file name. "
                                           "After choosing a file name, a job is created. "
                                           "When it is done, it automatically replaces clips, or you can double-click the job to open it.\n")),
                           producer->get_int("progressive"), parent);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.showCheckBox();
    dialog.set709Convert(!Util::trcIsCompatible(producer->get_int("meta.media.color_trc")));
    dialog.showSubClipCheckBox();
    LOG_DEBUG() << "in" << producer->get_in() << "out" << producer->get_out() << "length" <<
                producer->get_length() - 1;
    dialog.setSubClipChecked(producer->get_in() > 0
                             || producer->get_out() < producer->get_length() - 1);
    auto fps = Util::getAndroidFrameRate(producer);
    if (fps > 0.0)
        dialog.setFrameRate(fps);
    Transcoder transcoder;
    transcoder.addProducer(producer);
    transcoder.convert(dialog);
}

double Util::getAndroidFrameRate(Mlt::Producer *producer)
{
    auto fps = producer->get_double("meta.attr.com.android.capture.fps.markup");
    if (!qIsFinite(fps))
        fps = 0.0;
    return fps;
}

double Util::getSuggestedFrameRate(Mlt::Producer *producer)
{
    auto fps = producer->get_double("meta.attr.com.android.capture.fps.markup");
    if (!qIsFinite(fps))
        fps = 0.0;
    if (fps <= 0.0) {
        fps = producer->get_double("meta.media.frame_rate_num");
        if (producer->get_double("meta.media.frame_rate_den") > 0)
            fps /= producer->get_double("meta.media.frame_rate_den");
        if (producer->get("force_fps"))
            fps = producer->get_double("fps");
    }
    return fps;
}

Mlt::Producer Util::openMltVirtualClip(const QString &path)
{
    Mlt::Producer xmlProducer(nullptr, "xml-clip", path.toUtf8().constData());
    QScopedPointer<Mlt::Profile> testProfile(xmlProducer.profile());
    if (Settings.playerGPU() && MLT.profile().is_explicit()) {
        if (testProfile->width() != MLT.profile().width() || testProfile->height() != MLT.profile().height()
                || Util::isFpsDifferent(MLT.profile().fps(), testProfile->fps())) {
            return Mlt::Producer();
        }
    }
    if (xmlProducer.is_valid()) {
        Mlt::Chain chain(MLT.profile());
        chain.set_source(xmlProducer);
        chain.attach_normalizers();
        chain.get_length_time(mlt_time_clock);
        chain.set(kShotcutVirtualClip, 1);
        chain.set("resource", path.toUtf8().constData());
        return chain;
    }
    return Mlt::Producer();
}
