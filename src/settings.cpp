/*
 * Copyright (c) 2013-2026 Meltytech, LLC
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

#include "settings.h"

#include "Logger.h"
#include "qmltypes/qmlapplication.h"

#include <algorithm>

#include <QApplication>
#include <QAudioDevice>
#include <QColor>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMediaDevices>
#include <QStandardPaths>
#include <qdesktopservices.h>

static const QString APP_DATA_DIR_KEY("appdatadir");
static const QString SNAPFLOW_INI_FILENAME("/snapflow.ini");
static const QString RECENT_INI_FILENAME("recent.ini");
static QScopedPointer<SnapflowSettings> instance;
static QString appDataForSession;
static const int kMaximumTrackHeight = 125;
static const QString kRecentKey("recent");
static const QString kProjectsKey("projects");

namespace {
struct ModeMap
{
    SnapflowSettings::ProcessingMode id;
    const char *name;
};
static constexpr ModeMap kModeMap[] = {
    {SnapflowSettings::Native8Cpu, "Native8Cpu"},
    {SnapflowSettings::Linear8Cpu, "Linear8Cpu"},
    {SnapflowSettings::Native10Cpu, "Native10Cpu"},
    {SnapflowSettings::Linear10Cpu, "Linear10Cpu"},
    {SnapflowSettings::Linear10GpuCpu, "Linear10GpuCpu"},
};
} // anonymous namespace

SnapflowSettings &SnapflowSettings::singleton()
{
    if (!instance) {
        if (appDataForSession.isEmpty()) {
            instance.reset(new SnapflowSettings);
            if (instance->settings.value(APP_DATA_DIR_KEY).isValid()
                && QFile::exists(instance->settings.value(APP_DATA_DIR_KEY).toString()
                                 + SNAPFLOW_INI_FILENAME))
                instance.reset(
                    new SnapflowSettings(instance->settings.value(APP_DATA_DIR_KEY).toString()));
        } else {
            instance.reset(new SnapflowSettings(appDataForSession));
        }
    }
    return *instance;
}

/*!
    \qmltype Settings
    \inqmlmodule org.snapflow.qml
    \brief Persistent application settings, accessed via the \c settings context property.

    \c settings is an uncreatable QML type — use the global \c settings identifier
    available in every Snapflow QML view. Settings are backed by QSettings and persist
    across sessions.

    \code
    if (settings.timelineSnap) { ... }
    settings.timelineSnap = true
    \endcode
*/

/*!
    \qmlproperty Settings::TimelineScrolling Settings::timelineScrolling
    \brief The timeline auto-scroll mode.
    One of \c NoScrolling, \c CenterPlayhead, \c PageScrolling, or \c SmoothScrolling.
*/

SnapflowSettings::SnapflowSettings()
    : QObject()
    , m_recent(QDir(appDataLocation()).filePath(RECENT_INI_FILENAME), QSettings::IniFormat)
{
    migrateLayout();
    migrateRecent();
}

SnapflowSettings::SnapflowSettings(const QString &appDataLocation)
    : QObject()
    , settings(appDataLocation + SNAPFLOW_INI_FILENAME, QSettings::IniFormat)
    , m_appDataLocation(appDataLocation)
    , m_recent(QDir(appDataLocation).filePath(RECENT_INI_FILENAME), QSettings::IniFormat)
{
    migrateLayout();
    migrateRecent();
}

void SnapflowSettings::migrateRecent()
{
    // Migrate recent to separate INI file
    auto oldRecents = settings.value(kRecentKey).toStringList();
    if (recent().isEmpty() && !oldRecents.isEmpty()) {
        auto newRecents = recent();
        for (const auto &a : oldRecents) {
            if (a.size() < SnapflowSettings::MaxPath && !newRecents.contains(a)) {
                while (newRecents.size() > 100) {
                    newRecents.removeFirst();
                }
                newRecents.append(a);
            }
        }
        setRecent(newRecents);
        m_recent.sync();
        //        settings.remove("recent");
        settings.sync();
    }
}

void SnapflowSettings::migrateLayout()
{
    // Migrate old startup layout to a custom layout and start fresh
    if (!settings.contains("geometry2")) {
        auto geometry = settings.value("geometry").toByteArray();
        auto windowState = settings.value("windowState").toByteArray();
        setLayout(tr("Old (before v23) Layout"), geometry, windowState);
        setLayoutMode(2);
        settings.sync();
    }
}

void SnapflowSettings::log()
{
    LOG_INFO() << "language" << language();
    LOG_INFO() << "deinterlacer" << playerDeinterlacer();
    LOG_INFO() << "external monitor" << playerExternal();
    LOG_INFO() << "GPU processing" << playerGPU();
    LOG_INFO() << "interpolation" << playerInterpolation();
    LOG_INFO() << "video mode" << playerProfile();
    LOG_INFO() << "realtime" << playerRealtime();
    LOG_INFO() << "audio channels" << playerAudioChannels();
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (::qEnvironmentVariableIsSet("SDL_AUDIODRIVER")) {
        LOG_INFO() << "audio driver" << ::qgetenv("SDL_AUDIODRIVER");
    } else {
        LOG_INFO() << "audio driver" << playerAudioDriver();
    }
#endif
}

QString SnapflowSettings::language() const
{
    QString language = settings.value("language", QLocale().name()).toString();
    if (language == "en")
        language = "en_US";
    return language;
}

void SnapflowSettings::setLanguage(const QString &s)
{
    settings.setValue("language", s);
}

double SnapflowSettings::imageDuration() const
{
    return settings.value("imageDuration", 4.0).toDouble();
}

void SnapflowSettings::setImageDuration(double d)
{
    settings.setValue("imageDuration", d);
}

/*!
    \qmlproperty string Settings::openPath
    \brief The last directory used for opening files.
*/

QString SnapflowSettings::openPath() const
{
    return settings
        .value("openPath", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation))
        .toString();
}

void SnapflowSettings::setOpenPath(const QString &s)
{
    settings.setValue("openPath", s);
    emit savePathChanged();
}

/*!
    \qmlproperty string Settings::savePath
    \brief The last directory used for saving files.
*/

QString SnapflowSettings::savePath() const
{
    return settings
        .value("savePath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation))
        .toString();
}

void SnapflowSettings::setSavePath(const QString &s)
{
    settings.setValue("savePath", s);
    emit savePathChanged();
}

QStringList SnapflowSettings::recent() const
{
    return m_recent.value(kRecentKey).toStringList();
}

void SnapflowSettings::setRecent(const QStringList &ls)
{
    if (ls.isEmpty())
        m_recent.remove(kRecentKey);
    else if (!clearRecent())
        m_recent.setValue(kRecentKey, ls);
}

QStringList SnapflowSettings::projects()
{
    auto ls = m_recent.value(kProjectsKey).toStringList();
    if (ls.isEmpty()) {
        for (auto &r : recent()) {
            if (r.endsWith(".mlt"))
                ls << r;
        }
        // Prevent entering this block repeatedly
        if (ls.isEmpty())
            ls << QString();
        setProjects(ls);
    }
    return ls;
}

void SnapflowSettings::setProjects(const QStringList &ls)
{
    if (ls.isEmpty())
        m_recent.remove(kProjectsKey);
    else if (!clearRecent())
        m_recent.setValue(kProjectsKey, ls);
}

QString SnapflowSettings::theme() const
{
    return settings.value("theme", "dark").toString();
}

void SnapflowSettings::setTheme(const QString &s)
{
    settings.setValue("theme", s);
}

QThread::Priority SnapflowSettings::jobPriority() const
{
    const auto priority = settings.value("jobPriority", "low").toString();
    if (priority == "low") {
        return QThread::LowPriority;
    }
    return QThread::NormalPriority;
}

void SnapflowSettings::setJobPriority(const QString &s)
{
    settings.setValue("jobPriority", s);
}

bool SnapflowSettings::showTitleBars() const
{
    return settings.value("titleBars", true).toBool();
}

void SnapflowSettings::setShowTitleBars(bool b)
{
    settings.setValue("titleBars", b);
}

bool SnapflowSettings::showToolBar() const
{
    return settings.value("toolBar", true).toBool();
}

void SnapflowSettings::setShowToolBar(bool b)
{
    settings.setValue("toolBar", b);
}

bool SnapflowSettings::textUnderIcons() const
{
    return settings.value("textUnderIcons", true).toBool();
}

void SnapflowSettings::setTextUnderIcons(bool b)
{
    settings.setValue("textUnderIcons", b);
}

/*!
    \qmlproperty bool Settings::smallIcons
    \brief Whether the toolbar uses small icons.
*/

bool SnapflowSettings::smallIcons() const
{
    return settings.value("smallIcons", false).toBool();
}

void SnapflowSettings::setSmallIcons(bool b)
{
    settings.setValue("smallIcons", b);
    emit smallIconsChanged();
}

QByteArray SnapflowSettings::windowGeometry() const
{
    return settings.value("geometry2").toByteArray();
}

void SnapflowSettings::setWindowGeometry(const QByteArray &a)
{
    settings.setValue("geometry2", a);
}

QByteArray SnapflowSettings::windowGeometryDefault() const
{
    return settings.value("geometryDefault").toByteArray();
}

void SnapflowSettings::setWindowGeometryDefault(const QByteArray &a)
{
    settings.setValue("geometryDefault", a);
}

QByteArray SnapflowSettings::windowState() const
{
    return settings.value("windowState2").toByteArray();
}

void SnapflowSettings::setWindowState(const QByteArray &a)
{
    settings.setValue("windowState2", a);
}

QByteArray SnapflowSettings::windowStateDefault() const
{
    return settings.value("windowStateDefault").toByteArray();
}

void SnapflowSettings::setWindowStateDefault(const QByteArray &a)
{
    settings.setValue("windowStateDefault", a);
}

int SnapflowSettings::dockLayoutVersion() const
{
    return settings.value("dockLayoutVersion", 0).toInt();
}

void SnapflowSettings::setDockLayoutVersion(int v)
{
    settings.setValue("dockLayoutVersion", v);
}

/*!
    \qmlproperty string Settings::viewMode
    \brief The current view mode of the Playlist panel (e.g. \c "details", \c "icons").
*/

QString SnapflowSettings::viewMode() const
{
    return settings.value("playlist/viewMode").toString();
}

void SnapflowSettings::setViewMode(const QString &viewMode)
{
    settings.setValue("playlist/viewMode", viewMode);
    emit viewModeChanged();
}

QString SnapflowSettings::filesViewMode() const
{
    return settings.value("files/viewMode", QLatin1String("tiled")).toString();
}

void SnapflowSettings::setFilesViewMode(const QString &viewMode)
{
    settings.setValue("files/viewMode", viewMode);
    emit filesViewModeChanged();
}

QStringList SnapflowSettings::filesLocations() const
{
    QStringList result;
    for (const auto &s : settings.value("files/locations").toStringList()) {
        if (!s.startsWith("__"))
            result << s;
    }
    return result;
}

QString SnapflowSettings::filesLocationPath(const QString &name) const
{
    QString key = QStringLiteral("files/location/%1").arg(name);
    return settings.value(key).toString();
}

bool SnapflowSettings::setFilesLocation(const QString &name, const QString &path)
{
    bool isNew = false;
    QStringList locations = filesLocations();
    if (!locations.contains(name)) {
        isNew = true;
        locations.append(name);
        settings.setValue("files/locations", locations);
    }
    settings.setValue("files/location/" + name, path);
    return isNew;
}

bool SnapflowSettings::removeFilesLocation(const QString &name)
{
    QStringList list = filesLocations();
    int index = list.indexOf(name);
    if (index > -1) {
        list.removeAt(index);
        if (list.isEmpty())
            settings.remove("files/locations");
        else
            settings.setValue("files/locations", list);
        settings.remove("files/location/" + name);
        return true;
    }
    return false;
}

QStringList SnapflowSettings::filesOpenOther(const QString &type) const
{
    return settings.value("files/openOther/" + type).toStringList();
}

void SnapflowSettings::setFilesOpenOther(const QString &type, const QString &filePath)
{
    QStringList filePaths = filesOpenOther(type);
    filePaths.removeAll(filePath);
    filePaths.append(filePath);
    settings.setValue("files/openOther/" + type, filePaths);
}

bool SnapflowSettings::removeFilesOpenOther(const QString &type, const QString &filePath)
{
    QStringList list = filesOpenOther(type);
    int index = list.indexOf(filePath);
    if (index > -1) {
        list.removeAt(index);
        if (list.isEmpty())
            settings.remove("files/openOther/" + type);
        else
            settings.setValue("files/openOther/" + type, list);
        return true;
    }
    return false;
}

QString SnapflowSettings::filesCurrentDir() const
{
    const auto ls = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    auto path = settings.value("files/currentDir", ls.first()).toString();
    if (!QFile::exists(path)) {
        LOG_DEBUG() << "dir does not exist:" << QDir::toNativeSeparators(path);
        path = ls.first();
    }
    return path;
}

void SnapflowSettings::setFilesCurrentDir(const QString &s)
{
    settings.setValue("files/currentDir", s);
}

bool SnapflowSettings::filesFoldersOpen() const
{
    return settings.value("files/foldersOpen", true).toBool();
}

void SnapflowSettings::setFilesFoldersOpen(bool b)
{
    settings.setValue("files/foldersOpen", b);
}

QString SnapflowSettings::exportFrameSuffix() const
{
    return settings.value("exportFrameSuffix", ".png").toString();
}

void SnapflowSettings::setExportFrameSuffix(const QString &exportFrameSuffix)
{
    settings.setValue("exportFrameSuffix", exportFrameSuffix);
}

QString SnapflowSettings::encodePath() const
{
    return settings
        .value("encode/path", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation))
        .toString();
}

void SnapflowSettings::setEncodePath(const QString &s)
{
    settings.setValue("encode/path", s);
}

bool SnapflowSettings::encodeFreeSpaceCheck() const
{
    return settings.value("encode/freeSpaceCheck", true).toBool();
}

void SnapflowSettings::setEncodeFreeSpaceCheck(bool b)
{
    settings.setValue("encode/freeSpaceCheck", b);
}

bool SnapflowSettings::encodeUseHardware() const
{
    return settings.value("encode/useHardware").toBool();
}

void SnapflowSettings::setEncodeUseHardware(bool b)
{
    settings.setValue("encode/useHardware", b);
}

QStringList SnapflowSettings::encodeHardware() const
{
    return settings.value("encode/hardware").toStringList();
}

void SnapflowSettings::setEncodeHardware(const QStringList &ls)
{
    if (ls.isEmpty())
        settings.remove("encode/hardware");
    else
        settings.setValue("encode/hardware", ls);
}

bool SnapflowSettings::encodeHardwareDecoder() const
{
    return settings.value("encode/hardwareDecoder", false).toBool();
}

void SnapflowSettings::setEncodeHardwareDecoder(bool b)
{
    settings.setValue("encode/hardwareDecoder", b);
}

bool SnapflowSettings::encodeAdvanced() const
{
    return settings.value("encode/advanced", false).toBool();
}

void SnapflowSettings::setEncodeAdvanced(bool b)
{
    settings.setValue("encode/advanced", b);
}

bool SnapflowSettings::convertAdvanced() const
{
    return settings.value("convertAdvanced", false).toBool();
}

void SnapflowSettings::setConvertAdvanced(bool b)
{
    settings.setValue("convertAdvanced", b);
}

SnapflowSettings::ProcessingMode SnapflowSettings::processingMode()
{
    if (settings.contains("processingMode")) {
        auto result = (SnapflowSettings::ProcessingMode) settings.value("processingMode").toInt();
        if (result == Linear8Cpu) {
            // No longer supported but kept to prevent unexpected processing behavior going from
            // beta to release
            result = Native8Cpu;
        }
        return result;
    } else if (settings.contains("player/gpu2")) {
        // Legacy GPU Mode
        if (settings.value("player/gpu2").toBool()) {
            return SnapflowSettings::Linear10GpuCpu;
        }
    }
    return SnapflowSettings::Native8Cpu;
}

void SnapflowSettings::setProcessingMode(ProcessingMode mode)
{
    settings.setValue("processingMode", mode);
    emit playerGpuChanged();
}

QString SnapflowSettings::processingModeStr(SnapflowSettings::ProcessingMode mode)
{
    for (const auto &m : kModeMap) {
        if (m.id == mode)
            return QString::fromLatin1(m.name);
    }
    LOG_ERROR() << "Unknown processing mode" << mode;
    return QStringLiteral("Native8Cpu");
}

SnapflowSettings::ProcessingMode SnapflowSettings::processingModeId(const QString &mode)
{
    for (const auto &m : kModeMap) {
        if (mode == QLatin1String(m.name))
            return m.id;
    }
    LOG_ERROR() << "Unknown processing mode" << mode;
    return Native8Cpu;
}

bool SnapflowSettings::isHdrCompatibleProcessingMode()
{
    const auto mode = processingMode();
    return mode == Native10Cpu || mode == Linear10GpuCpu;
}

bool SnapflowSettings::showConvertClipDialog() const
{
    return settings.value("showConvertClipDialog", true).toBool();
}

void SnapflowSettings::setShowConvertClipDialog(bool b)
{
    settings.setValue("showConvertClipDialog", b);
}

bool SnapflowSettings::showHdrPlayerWarning() const
{
    return settings.value("showHdrPlayerWarning", true).toBool();
}

void SnapflowSettings::setShowHdrPlayerWarning(bool b)
{
    settings.setValue("showHdrPlayerWarning", b);
}

bool SnapflowSettings::encodeParallelProcessing() const
{
    return settings.value("encode/parallelProcessing", false).toBool();
}

void SnapflowSettings::setEncodeParallelProcessing(bool b)
{
    settings.setValue("encode/parallelProcessing", b);
}

/*!
    \qmlproperty int Settings::playerAudioChannels
    \brief The number of audio channels used by the player (e.g. 2 or 6).
*/

int SnapflowSettings::playerAudioChannels() const
{
    return settings.value("player/audioChannels", 2).toInt();
}

void SnapflowSettings::setPlayerAudioChannels(int i)
{
    settings.setValue("player/audioChannels", i);
    emit playerAudioChannelsChanged(i);
}

QString SnapflowSettings::playerDeinterlacer() const
{
    QString result = settings.value("player/deinterlacer", "onefield").toString();
    //XXX workaround yadif crashing with mlt_transition
    if (result == "yadif" || result == "yadif-nospatial")
        result = "onefield";
    return result;
}

void SnapflowSettings::setPlayerDeinterlacer(const QString &s)
{
    settings.setValue("player/deinterlacer", s);
}

QString SnapflowSettings::playerExternal() const
{
    auto result = settings.value("player/external", "").toString();
    // "sdi" is no longer supported DVEO VidPort
    return result == "sdi" ? "" : result;
}

void SnapflowSettings::setPlayerExternal(const QString &s)
{
    settings.setValue("player/external", s);
}

bool SnapflowSettings::playerJACK() const
{
    return settings.value("player/jack", false).toBool();
}

QString SnapflowSettings::playerInterpolation() const
{
    return settings.value("player/interpolation", "bilinear").toString();
}

void SnapflowSettings::setPlayerInterpolation(const QString &s)
{
    settings.setValue("player/interpolation", s);
}

/*!
    \qmlproperty bool Settings::playerGPU
    \brief Whether GPU processing (GLSL) is enabled for the video player.
*/

bool SnapflowSettings::playerGPU() const
{
    // This is the legacy function for the old GPU mode.
    if (settings.contains("processingMode")) {
        ProcessingMode mode = (ProcessingMode) settings.value("processingMode").toInt();
        return mode == Linear10GpuCpu;
    } else if (settings.contains("player/gpu2")) {
        // Legacy GPU Mode
        return settings.value("player/gpu2").toBool();
    }
    return false;
}

bool SnapflowSettings::playerWarnGPU() const
{
    return false; //settings.value("player/warnGPU", false).toBool();
}

void SnapflowSettings::setPlayerJACK(bool b)
{
    settings.setValue("player/jack", b);
}

int SnapflowSettings::playerDecklinkHdrMaxCll() const
{
    return settings.value("player/decklinkHdrMaxCll", 1000).toInt();
}

void SnapflowSettings::setPlayerDecklinkHdrMaxCll(int nits)
{
    settings.setValue("player/decklinkHdrMaxCll", nits);
}

int SnapflowSettings::playerDecklinkHdrMaxFall() const
{
    return settings.value("player/decklinkHdrMaxFall", 400).toInt();
}

void SnapflowSettings::setPlayerDecklinkHdrMaxFall(int nits)
{
    settings.setValue("player/decklinkHdrMaxFall", nits);
}

int SnapflowSettings::playerDecklinkHdrMasterPreset() const
{
    return settings.value("player/decklinkHdrMasterPreset", 0).toInt();
}

void SnapflowSettings::setPlayerDecklinkHdrMasterPreset(int preset)
{
    settings.setValue("player/decklinkHdrMasterPreset", preset);
}

int SnapflowSettings::playerDecklinkHdrMaxLuminance() const
{
    return settings.value("player/decklinkHdrMaxLuminance", 1000).toInt();
}

void SnapflowSettings::setPlayerDecklinkHdrMaxLuminance(int nits)
{
    settings.setValue("player/decklinkHdrMaxLuminance", nits);
}

double SnapflowSettings::playerDecklinkHdrMinLuminance() const
{
    return settings.value("player/decklinkHdrMinLuminance", 0.01).toDouble();
}

void SnapflowSettings::setPlayerDecklinkHdrMinLuminance(double nits)
{
    settings.setValue("player/decklinkHdrMinLuminance", nits);
}

int SnapflowSettings::playerKeyerMode() const
{
    return settings.value("player/keyer", 0).toInt();
}

void SnapflowSettings::setPlayerKeyerMode(int i)
{
    settings.setValue("player/keyer", i);
}

bool SnapflowSettings::playerMuted() const
{
    return settings.value("player/muted", false).toBool();
}

void SnapflowSettings::setPlayerMuted(bool b)
{
    settings.setValue("player/muted", b);
}

QString SnapflowSettings::playerProfile() const
{
    return settings.value("player/profile", "").toString();
}

void SnapflowSettings::setPlayerProfile(const QString &s)
{
    settings.setValue("player/profile", s);
}

bool SnapflowSettings::playerProgressive() const
{
    return settings.value("player/progressive", true).toBool();
}

void SnapflowSettings::setPlayerProgressive(bool b)
{
    settings.setValue("player/progressive", b);
}

bool SnapflowSettings::playerRealtime() const
{
    return settings.value("player/realtime", true).toBool();
}

void SnapflowSettings::setPlayerRealtime(bool b)
{
    settings.setValue("player/realtime", b);
}

bool SnapflowSettings::playerScrubAudio() const
{
    return settings.value("player/scrubAudio", true).toBool();
}

void SnapflowSettings::setPlayerScrubAudio(bool b)
{
    settings.setValue("player/scrubAudio", b);
}

int SnapflowSettings::playerVolume() const
{
    return settings.value("player/volume", 88).toInt();
}

void SnapflowSettings::setPlayerVolume(int i)
{
    settings.setValue("player/volume", i);
}

float SnapflowSettings::playerZoom() const
{
    return settings.value("player/zoom", 0.0f).toFloat();
}

void SnapflowSettings::setPlayerZoom(float f)
{
    settings.setValue("player/zoom", f);
}

int SnapflowSettings::playerPreviewScale() const
{
    return settings.value("player/previewScale", 0).toInt();
}

void SnapflowSettings::setPlayerPreviewScale(int i)
{
    settings.setValue("player/previewScale", i);
}

bool SnapflowSettings::playerPreviewHardwareDecoder() const
{
    return settings.value("player/previewHardwareDecoder", true).toBool();
}

bool SnapflowSettings::playerPreviewHardwareDecoderIsSet() const
{
    return settings.contains("player/previewHardwareDecoder");
}

void SnapflowSettings::setPlayerPreviewHardwareDecoder(bool b)
{
    settings.setValue("player/previewHardwareDecoder", b);
}

int SnapflowSettings::playerVideoDelayMs() const
{
    return settings.value("player/videoDelayMs", 0).toInt();
}

void SnapflowSettings::setPlayerVideoDelayMs(int i)
{
    settings.setValue("player/videoDelayMs", i);
}

double SnapflowSettings::playerJumpSeconds() const
{
    return settings.value("player/jumpSeconds", 60.0).toDouble();
}

void SnapflowSettings::setPlayerJumpSeconds(double i)
{
    settings.setValue("player/jumpSeconds", i);
}

QString SnapflowSettings::playerAudioDriver() const
{
#if defined(Q_OS_WIN)
    auto s = playerAudioChannels() > 2 ? "directsound" : "winmm";
#else
    auto s = "pulseaudio";
#endif
    if (::qEnvironmentVariableIsSet("SDL_AUDIODRIVER")) {
        return ::qgetenv("SDL_AUDIODRIVER");
    } else {
        return settings.value("player/audioDriver", s).toString();
    }
}

void SnapflowSettings::setPlayerAudioDriver(const QString &s)
{
    settings.setValue("player/audioDriver", s);
}

bool SnapflowSettings::playerPauseAfterSeek() const
{
    return settings.value("player/pauseAfterSeek", true).toBool();
}

void SnapflowSettings::setPlayerPauseAfterSeek(bool b)
{
    settings.setValue("player/pauseAfterSeek", b);
}

bool SnapflowSettings::playerOldVideoOutput() const
{
    return settings.value("player/oldVideoOutput", false).toBool();
}

void SnapflowSettings::setPlayerOldVideoOutput(bool b)
{
    settings.setValue("player/oldVideoOutput", b);
}

bool SnapflowSettings::playerHdrPreview() const
{
    return settings.value("player/hdrPreview", false).toBool();
}

void SnapflowSettings::setPlayerHdrPreview(bool b)
{
    settings.setValue("player/hdrPreview", b);
}

QRect SnapflowSettings::playerHdrPreviewGeometry() const
{
    return settings.value("player/hdrPreviewGeometry").toRect();
}

void SnapflowSettings::setPlayerHdrPreviewGeometry(const QRect &r)
{
    settings.setValue("player/hdrPreviewGeometry", r);
}

bool SnapflowSettings::playerHdrPreviewFullScreen() const
{
    return settings.value("player/hdrPreviewFullScreen", false).toBool();
}

void SnapflowSettings::setPlayerHdrPreviewFullScreen(bool b)
{
    settings.setValue("player/hdrPreviewFullScreen", b);
}

int SnapflowSettings::playerHdrDisplayPeakNits() const
{
    return settings.value("player/hdrDisplayPeakNits", 0).toInt();
}

void SnapflowSettings::setPlayerHdrDisplayPeakNits(int nits)
{
    settings.setValue("player/hdrDisplayPeakNits", nits);
}

int SnapflowSettings::playerHdrContentPeakNits() const
{
    return settings.value("player/hdrContentPeakNits", 0).toInt();
}

void SnapflowSettings::setPlayerHdrContentPeakNits(int nits)
{
    settings.setValue("player/hdrContentPeakNits", nits);
}

bool SnapflowSettings::playerHdrToneMapping() const
{
    return settings.value("player/hdrToneMapping", true).toBool();
}

void SnapflowSettings::setPlayerHdrToneMapping(bool b)
{
    settings.setValue("player/hdrToneMapping", b);
}

/*!
    \qmlproperty string Settings::playlistThumbnails
    \brief The thumbnail display mode for the Playlist panel.
*/

QString SnapflowSettings::playlistThumbnails() const
{
    return settings.value("playlist/thumbnails", "small").toString();
}

void SnapflowSettings::setPlaylistThumbnails(const QString &s)
{
    settings.setValue("playlist/thumbnails", s);
    emit playlistThumbnailsChanged();
}

bool SnapflowSettings::playlistAutoplay() const
{
    return settings.value("playlist/autoplay", true).toBool();
}

void SnapflowSettings::setPlaylistAutoplay(bool b)
{
    settings.setValue("playlist/autoplay", b);
}

bool SnapflowSettings::playlistShowColumn(const QString &column)
{
    return settings.value("playlist/columns/" + column, true).toBool();
}

void SnapflowSettings::setPlaylistShowColumn(const QString &column, bool b)
{
    settings.setValue("playlist/columns/" + column, b);
}

/*!
    \qmlproperty bool Settings::timelineDragScrub
    \brief Whether scrubbing occurs while dragging clips on the timeline.
*/

bool SnapflowSettings::timelineDragScrub() const
{
    return settings.value("timeline/dragScrub", false).toBool();
}

void SnapflowSettings::setTimelineDragScrub(bool b)
{
    settings.setValue("timeline/dragScrub", b);
    emit timelineDragScrubChanged();
}

/*!
    \qmlproperty bool Settings::timelineShowWaveforms
    \brief Whether audio waveforms are shown on timeline clips.
*/

bool SnapflowSettings::timelineShowWaveforms() const
{
    return settings.value("timeline/waveforms", true).toBool();
}

void SnapflowSettings::setTimelineShowWaveforms(bool b)
{
    settings.setValue("timeline/waveforms", b);
    emit timelineShowWaveformsChanged();
}

/*!
    \qmlproperty bool Settings::timelineShowThumbnails
    \brief Whether video thumbnails are shown on timeline clips.
*/

bool SnapflowSettings::timelineShowThumbnails() const
{
    return settings.value("timeline/thumbnails", true).toBool();
}

void SnapflowSettings::setTimelineShowThumbnails(bool b)
{
    settings.setValue("timeline/thumbnails", b);
    emit timelineShowThumbnailsChanged();
}

/*!
    \qmlproperty bool Settings::timelineRipple
    \brief Whether ripple editing is enabled on the timeline.
*/

bool SnapflowSettings::timelineRipple() const
{
    return settings.value("timeline/ripple", false).toBool();
}

void SnapflowSettings::setTimelineRipple(bool b)
{
    settings.setValue("timeline/ripple", b);
    emit timelineRippleChanged();
}

/*!
    \qmlproperty bool Settings::timelineRippleAllTracks
    \brief Whether ripple editing affects all tracks simultaneously.
*/

bool SnapflowSettings::timelineRippleAllTracks() const
{
    return settings.value("timeline/rippleAllTracks", false).toBool();
}

void SnapflowSettings::setTimelineRippleAllTracks(bool b)
{
    settings.setValue("timeline/rippleAllTracks", b);
    emit timelineRippleAllTracksChanged();
}

/*!
    \qmlproperty bool Settings::timelineRippleMarkers
    \brief Whether markers are moved along with ripple edits.
*/

bool SnapflowSettings::timelineRippleMarkers() const
{
    return settings.value("timeline/rippleMarkers", false).toBool();
}

void SnapflowSettings::setTimelineRippleMarkers(bool b)
{
    settings.setValue("timeline/rippleMarkers", b);
    emit timelineRippleMarkersChanged();
}

/*!
    \qmlproperty bool Settings::timelineSnap
    \brief Whether clip snapping is enabled on the timeline.
*/

bool SnapflowSettings::timelineSnap() const
{
    return settings.value("timeline/snap", true).toBool();
}

void SnapflowSettings::setTimelineSnap(bool b)
{
    settings.setValue("timeline/snap", b);
    emit timelineSnapChanged();
}

int SnapflowSettings::timelineTrackHeight() const
{
    return qMin(settings.value("timeline/trackHeight", 50).toInt(), kMaximumTrackHeight);
}

void SnapflowSettings::setTimelineTrackHeight(int n)
{
    settings.setValue("timeline/trackHeight", qMin(n, kMaximumTrackHeight));
}

/*!
    \qmlproperty bool Settings::timelineScrollZoom
    \brief Whether the scroll wheel zooms the timeline (instead of scrolling).
*/

bool SnapflowSettings::timelineScrollZoom() const
{
    return settings.value("timeline/scrollZoom", true).toBool();
}

void SnapflowSettings::setTimelineScrollZoom(bool b)
{
    settings.setValue("timeline/scrollZoom", b);
    emit timelineScrollZoomChanged();
}

/*!
    \qmlproperty bool Settings::timelineFramebufferWaveform
    \brief Whether waveforms are rendered using a framebuffer (GPU) path.
*/

bool SnapflowSettings::timelineFramebufferWaveform() const
{
    return settings.value("timeline/framebufferWaveform", true).toBool();
}

void SnapflowSettings::setTimelineFramebufferWaveform(bool b)
{
    settings.setValue("timeline/framebufferWaveform", b);
    emit timelineFramebufferWaveformChanged();
}

int SnapflowSettings::audioReferenceTrack() const
{
    return settings.value("timeline/audioReferenceTrack", 0).toInt();
}
void SnapflowSettings::setAudioReferenceTrack(int track)
{
    settings.setValue("timeline/audioReferenceTrack", track);
}

double SnapflowSettings::audioReferenceSpeedRange() const
{
    return settings.value("timeline/audioReferenceSpeedRange", 0).toDouble();
}
void SnapflowSettings::setAudioReferenceSpeedRange(double range)
{
    settings.setValue("timeline/audioReferenceSpeedRange", range);
}

bool SnapflowSettings::timelinePreviewTransition() const
{
    return settings.value("timeline/previewTransition", true).toBool();
}

void SnapflowSettings::setTimelinePreviewTransition(bool b)
{
    settings.setValue("timeline/previewTransition", b);
}

void SnapflowSettings::setTimelineScrolling(SnapflowSettings::TimelineScrolling value)
{
    settings.remove("timeline/centerPlayhead");
    settings.setValue("timeline/scrolling", value);
    emit timelineScrollingChanged();
}

SnapflowSettings::TimelineScrolling SnapflowSettings::timelineScrolling() const
{
    if (settings.contains("timeline/centerPlayhead")
        && settings.value("timeline/centerPlayhead").toBool())
        return SnapflowSettings::TimelineScrolling::CenterPlayhead;
    else
        return SnapflowSettings::TimelineScrolling(
            settings.value("timeline/scrolling", PageScrolling).toInt());
}

bool SnapflowSettings::timelineAutoAddTracks() const
{
    return settings.value("timeline/autoAddTracks", false).toBool();
}

void SnapflowSettings::setTimelineAutoAddTracks(bool b)
{
    if (b != timelineAutoAddTracks()) {
        settings.setValue("timeline/autoAddTracks", b);
        emit timelineAutoAddTracksChanged();
    }
}

/*!
    \qmlproperty bool Settings::timelineRectangleSelect
    \brief Whether rectangle (rubber-band) selection is enabled on the timeline.
*/

bool SnapflowSettings::timelineRectangleSelect() const
{
    return settings.value("timeline/rectangleSelect", true).toBool();
}

void SnapflowSettings::setTimelineRectangleSelect(bool b)
{
    settings.setValue("timeline/rectangleSelect", b);
    emit timelineRectangleSelectChanged();
}

/*!
    \qmlproperty bool Settings::timelineAdjustGain
    \brief Whether dragging the gain handle on audio clips adjusts volume inline.
*/

bool SnapflowSettings::timelineAdjustGain() const
{
    return settings.value("timeline/adjustGain", false).toBool();
}

void SnapflowSettings::setTimelineAdjustGain(bool b)
{
    settings.setValue("timeline/adjustGain", b);
    emit timelineAdjustGainChanged();
}

/*!
    \qmlproperty bool Settings::timelineAllowTransitions
    \brief Whether overlapping clips on the timeline automatically create transitions.
*/

bool SnapflowSettings::timelineAllowTransitions() const
{
    return settings.value("timeline/allowTransitions", true).toBool();
}

void SnapflowSettings::setTimelineAllowTransitions(bool b)
{
    if (b != timelineAllowTransitions()) {
        settings.setValue("timeline/allowTransitions", b);
        emit timelineAllowTransitionsChanged();
    }
}

QString SnapflowSettings::filterFavorite(const QString &filterName)
{
    return settings.value("filter/favorite/" + filterName, "").toString();
}

void SnapflowSettings::setFilterFavorite(const QString &filterName, const QString &value)
{
    settings.setValue("filter/favorite/" + filterName, value);
}

QStringList SnapflowSettings::addOnFilterServices() const
{
    return settings.value("filter/addOnServices").toStringList();
}

void SnapflowSettings::setAddOnFilterServices(const QStringList &services)
{
    settings.setValue("filter/addOnServices", services);
}

/*!
    \qmlproperty real Settings::audioInDuration
    \brief The default duration in seconds for audio fade-in transitions.
*/

double SnapflowSettings::audioInDuration() const
{
    return settings.value("filter/audioInDuration", 1.0).toDouble();
}

void SnapflowSettings::setAudioInDuration(double d)
{
    settings.setValue("filter/audioInDuration", d);
    emit audioInDurationChanged();
}

/*!
    \qmlproperty real Settings::audioOutDuration
    \brief The default duration in seconds for audio fade-out transitions.
*/

double SnapflowSettings::audioOutDuration() const
{
    return settings.value("filter/audioOutDuration", 1.0).toDouble();
}

void SnapflowSettings::setAudioOutDuration(double d)
{
    settings.setValue("filter/audioOutDuration", d);
    emit audioOutDurationChanged();
}

/*!
    \qmlproperty real Settings::videoInDuration
    \brief The default duration in seconds for video fade-in transitions.
*/

double SnapflowSettings::videoInDuration() const
{
    return settings.value("filter/videoInDuration", 1.0).toDouble();
}

void SnapflowSettings::setVideoInDuration(double d)
{
    settings.setValue("filter/videoInDuration", d);
    emit videoInDurationChanged();
}

/*!
    \qmlproperty real Settings::videoOutDuration
    \brief The default duration in seconds for video fade-out transitions.
*/

double SnapflowSettings::videoOutDuration() const
{
    return settings.value("filter/videoOutDuration", 1.0).toDouble();
}

void SnapflowSettings::setVideoOutDuration(double d)
{
    settings.setValue("filter/videoOutDuration", d);
    emit videoOutDurationChanged();
}

/*!
    \qmlproperty int Settings::audioInCurve
    \brief The curve type for audio fade-in (0 = linear, higher = more exponential).
*/

int SnapflowSettings::audioInCurve() const
{
    return settings.value("filter/audioInCurve", mlt_keyframe_linear).toInt();
}

void SnapflowSettings::setAudioInCurve(int c)
{
    settings.setValue("filter/audioInCurve", c);
    emit audioInCurveChanged();
}

/*!
    \qmlproperty int Settings::audioOutCurve
    \brief The curve type for audio fade-out (0 = linear, higher = more exponential).
*/

int SnapflowSettings::audioOutCurve() const
{
    return settings.value("filter/audioOutCurve", mlt_keyframe_linear).toInt();
}

void SnapflowSettings::setAudioOutCurve(int c)
{
    settings.setValue("filter/audioOutCurve", c);
    emit audioOutCurveChanged();
}

/*!
    \qmlproperty bool Settings::askOutputFilter
    \brief Whether Snapflow should prompt before applying a filter to the output node.
*/

bool SnapflowSettings::askOutputFilter() const
{
    return settings.value("filter/askOutput", true).toBool();
}

void SnapflowSettings::setAskOutputFilter(bool b)
{
    settings.setValue("filter/askOutput", b);
    emit askOutputFilterChanged();
}

bool SnapflowSettings::loudnessScopeShowMeter(const QString &meter) const
{
    return settings.value("scope/loudness/" + meter, true).toBool();
}

void SnapflowSettings::setLoudnessScopeShowMeter(const QString &meter, bool b)
{
    settings.setValue("scope/loudness/" + meter, b);
}

void SnapflowSettings::setMarkerColor(const QColor &color)
{
    settings.setValue("markers/color", color.name());
}

QColor SnapflowSettings::markerColor() const
{
    return QColor(settings.value("markers/color", "green").toString());
}

void SnapflowSettings::setMarkersShowColumn(const QString &column, bool b)
{
    settings.setValue("markers/columns/" + column, b);
}

bool SnapflowSettings::markersShowColumn(const QString &column) const
{
    return settings.value("markers/columns/" + column, true).toBool();
}

void SnapflowSettings::setMarkerSort(int column, Qt::SortOrder order)
{
    settings.setValue("markers/sortColumn", column);
    settings.setValue("markers/sortOrder", order);
}

int SnapflowSettings::getMarkerSortColumn()
{
    return settings.value("markers/sortColumn", -1).toInt();
}

Qt::SortOrder SnapflowSettings::getMarkerSortOrder()
{
    return (Qt::SortOrder) settings.value("markers/sortOrder", Qt::AscendingOrder).toInt();
}

int SnapflowSettings::drawMethod() const
{
#ifdef Q_OS_WIN
    return settings.value("opengl", Qt::AA_UseOpenGLES).toInt();
#else
    return settings.value("opengl", Qt::AA_UseDesktopOpenGL).toInt();
#endif
}

void SnapflowSettings::setDrawMethod(int i)
{
    settings.setValue("opengl", i);
}

uint SnapflowSettings::gpuAdapterVendorId() const
{
    // PCI vendor id of the selected GPU (0x10DE NVIDIA, 0x1002 AMD, 0x8086 Intel).
    // 0 means Automatic / system default. The vendor+device pair is the stable identity
    // of the chosen GPU; the live DXGI adapter index is resolved from it at startup.
    return settings.value("player/gpuAdapterVendorId", 0).toUInt();
}

void SnapflowSettings::setGpuAdapterVendorId(uint id)
{
    settings.setValue("player/gpuAdapterVendorId", id);
}

uint SnapflowSettings::gpuAdapterDeviceId() const
{
    // PCI device id of the selected GPU; pairs with the vendor id to identify it.
    return settings.value("player/gpuAdapterDeviceId", 0).toUInt();
}

void SnapflowSettings::setGpuAdapterDeviceId(uint id)
{
    settings.setValue("player/gpuAdapterDeviceId", id);
}

bool SnapflowSettings::safeMode() const
{
    return settings.value("safeMode", false).toBool();
}

void SnapflowSettings::setSafeMode(bool value)
{
    settings.setValue("safeMode", value);
}

bool SnapflowSettings::noUpgrade() const
{
    return settings.value("noupgrade", false).toBool();
}

void SnapflowSettings::setNoUpgrade(bool value)
{
    settings.setValue("noupgrade", value);
}

bool SnapflowSettings::checkUpgradeAutomatic()
{
    return settings.value("checkUpgradeAutomatic", false).toBool();
}

void SnapflowSettings::setCheckUpgradeAutomatic(bool b)
{
    settings.setValue("checkUpgradeAutomatic", b);
}

bool SnapflowSettings::askUpgradeAutomatic()
{
    return settings.value("askUpgradeAutmatic", true).toBool();
}

void SnapflowSettings::setAskUpgradeAutomatic(bool b)
{
    settings.setValue("askUpgradeAutmatic", b);
}

bool SnapflowSettings::askChangeVideoMode()
{
    return settings.value("askChangeVideoMode", true).toBool();
}

void SnapflowSettings::setAskChangeVideoMode(bool b)
{
    settings.setValue("askChangeVideoMode", b);
}

void SnapflowSettings::sync()
{
    settings.sync();
}

/*!
    \qmlproperty string Settings::appDataLocation
    \brief The path to the application data directory (read-only).
*/

QString SnapflowSettings::appDataLocation() const
{
    if (!m_appDataLocation.isEmpty())
        return m_appDataLocation;
    else
        return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

void SnapflowSettings::setAppDataForSession(const QString &location)
{
    // This is intended to be called when using a command line option
    // to set the AppData location.
    appDataForSession = location;
    if (instance)
        instance.reset(new SnapflowSettings(location));
}

void SnapflowSettings::setAppDataLocally(const QString &location)
{
    // This is intended to be called when using a GUI action to set the
    // the new AppData location.

    // Copy the existing settings if they exist.
    if (!QFile::exists(location + SNAPFLOW_INI_FILENAME)) {
        QSettings newSettings(location + SNAPFLOW_INI_FILENAME, QSettings::IniFormat);
        foreach (const QString &key, settings.allKeys())
            newSettings.setValue(key, settings.value(key));
        newSettings.sync();
    }

    // Set the new location.
    QSettings localSettings;
    localSettings.setValue(APP_DATA_DIR_KEY, location);
    localSettings.sync();
}

QStringList SnapflowSettings::layouts() const
{
    QStringList result;
    for (const auto &s : settings.value("layout/layouts").toStringList()) {
        if (!s.startsWith("__"))
            result << s;
    }
    return result;
}

bool SnapflowSettings::setLayout(const QString &name,
                                const QByteArray &geometry,
                                const QByteArray &state)
{
    bool isNew = false;
    QStringList layouts = this->layouts();
    if (layouts.indexOf(name) == -1) {
        isNew = true;
        layouts.append(name);
        settings.setValue("layout/layouts", layouts);
    }
    settings.setValue(QStringLiteral("layout/%1_%2").arg(name, "geometry"), geometry);
    settings.setValue(QStringLiteral("layout/%1_%2").arg(name, "state"), state);
    return isNew;
}

QByteArray SnapflowSettings::layoutGeometry(const QString &name)
{
    QString key = QStringLiteral("layout/%1_geometry").arg(name);
    return settings.value(key).toByteArray();
}

QByteArray SnapflowSettings::layoutState(const QString &name)
{
    QString key = QStringLiteral("layout/%1_state").arg(name);
    return settings.value(key).toByteArray();
}

bool SnapflowSettings::removeLayout(const QString &name)
{
    QStringList list = layouts();
    int index = list.indexOf(name);
    if (index > -1) {
        list.removeAt(index);
        if (list.isEmpty())
            settings.remove("layout/layouts");
        else
            settings.setValue("layout/layouts", list);
        settings.remove(QStringLiteral("layout/%1_%2").arg(name, "geometry"));
        settings.remove(QStringLiteral("layout/%1_%2").arg(name, "state"));
        return true;
    }
    return false;
}

int SnapflowSettings::layoutMode() const
{
    return settings.value("layout/mode", -1).toInt();
}

void SnapflowSettings::setLayoutMode(int mode)
{
    settings.setValue("layout/mode", mode);
}

bool SnapflowSettings::clearRecent() const
{
    return settings.value("clearRecent", false).toBool();
}

void SnapflowSettings::setClearRecent(bool b)
{
    settings.setValue("clearRecent", b);
}

QString SnapflowSettings::projectsFolder() const
{
    return settings
        .value("projectsFolder", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation))
        .toString();
}

void SnapflowSettings::setProjectsFolder(const QString &path)
{
    settings.setValue("projectsFolder", path);
}

QString SnapflowSettings::audioInput() const
{
    QString defaultValue = "default";
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    for (const auto &deviceInfo : QMediaDevices::audioInputs()) {
        defaultValue = deviceInfo.description();
    }
#endif
    return settings.value("audioInput", defaultValue).toString();
}

void SnapflowSettings::setAudioInput(const QString &name)
{
    settings.setValue("audioInput", name);
}

QString SnapflowSettings::videoInput() const
{
    return settings.value("videoInput").toString();
}

void SnapflowSettings::setVideoInput(const QString &name)
{
    settings.setValue("videoInput", name);
}

QString SnapflowSettings::glaxnimatePath() const
{
    QDir dir(qApp->applicationDirPath());
    return settings.value("glaxnimatePath", dir.absoluteFilePath("glaxnimate")).toString();
}

void SnapflowSettings::setGlaxnimatePath(const QString &path)
{
    settings.setValue("glaxnimatePath", path);
}

void SnapflowSettings::resetGlaxnimatePath()
{
    settings.remove("glaxnimatePath");
}

bool SnapflowSettings::exportRangeMarkers() const
{
    return settings.value("exportRangeMarkers", true).toBool();
}

void SnapflowSettings::setExportRangeMarkers(bool b)
{
    settings.setValue("exportRangeMarkers", b);
}

bool SnapflowSettings::proxyEnabled() const
{
    return settings.value("proxy/enabled", false).toBool();
}

void SnapflowSettings::setProxyEnabled(bool b)
{
    settings.setValue("proxy/enabled", b);
}

QString SnapflowSettings::proxyFolder() const
{
    QDir dir(appDataLocation());
    const char *subfolder = "proxies";
    if (!dir.cd(subfolder)) {
        if (dir.mkdir(subfolder))
            dir.cd(subfolder);
    }
    return settings.value("proxy/folder", dir.path()).toString();
}

void SnapflowSettings::setProxyFolder(const QString &path)
{
    settings.setValue("proxy/folder", path);
}

bool SnapflowSettings::proxyUseProjectFolder() const
{
    return settings.value("proxy/useProjectFolder", true).toBool();
}

void SnapflowSettings::setProxyUseProjectFolder(bool b)
{
    settings.setValue("proxy/useProjectFolder", b);
}

bool SnapflowSettings::proxyUseHardware() const
{
    return settings.value("proxy/useHardware", false).toBool();
}

void SnapflowSettings::setProxyUseHardware(bool b)
{
    settings.setValue("proxy/useHardware", b);
}

void SnapflowSettings::clearShortcuts(const QString &name)
{
    QString key = "shortcuts/" + name;
    settings.remove(key);
}

void SnapflowSettings::setShortcuts(const QString &name, const QList<QKeySequence> &shortcuts)
{
    QString key = "shortcuts/" + name;
    QString shortcutSetting;
    if (shortcuts.size() > 0)
        shortcutSetting += shortcuts[0].toString();
    shortcutSetting += "||";
    if (shortcuts.size() > 1)
        shortcutSetting += shortcuts[1].toString();
    settings.setValue(key, shortcutSetting);
}

QList<QKeySequence> SnapflowSettings::shortcuts(const QString &name)
{
    QString key = "shortcuts/" + name;
    QList<QKeySequence> shortcuts;
    QString shortcutSetting = settings.value(key, "").toString();
    if (!shortcutSetting.isEmpty()) {
        for (const QString &s : shortcutSetting.split("||"))
            shortcuts << QKeySequence::fromString(s);
    }
    return shortcuts;
}

double SnapflowSettings::slideshowImageDuration(double defaultSeconds) const
{
    return settings.value("slideshow/clipDuration", defaultSeconds).toDouble();
}

void SnapflowSettings::setSlideshowImageDuration(double seconds)
{
    settings.setValue("slideshow/clipDuration", seconds);
}

double SnapflowSettings::slideshowAudioVideoDuration(double defaultSeconds) const
{
    return settings.value("slideshow/audioVideoDuration", defaultSeconds).toDouble();
}

void SnapflowSettings::setSlideshowAudioVideoDuration(double seconds)
{
    settings.setValue("slideshow/audioVideoDuration", seconds);
}

int SnapflowSettings::slideshowAspectConversion(int defaultAspectConversion) const
{
    return settings.value("slideshow/aspectConversion", defaultAspectConversion).toInt();
}

void SnapflowSettings::setSlideshowAspectConversion(int aspectConversion)
{
    settings.setValue("slideshow/aspectConversion", aspectConversion);
}

int SnapflowSettings::slideshowZoomPercent(int defaultZoomPercent) const
{
    return settings.value("slideshow/zoomPercent", defaultZoomPercent).toInt();
}

void SnapflowSettings::setSlideshowZoomPercent(int zoomPercent)
{
    settings.setValue("slideshow/zoomPercent", zoomPercent);
}

double SnapflowSettings::slideshowTransitionDuration(double defaultTransitionDuration) const
{
    return settings.value("slideshow/transitionDuration", defaultTransitionDuration).toDouble();
}

void SnapflowSettings::setSlideshowTransitionDuration(double transitionDuration)
{
    settings.setValue("slideshow/transitionDuration", transitionDuration);
}

int SnapflowSettings::slideshowTransitionStyle(int defaultTransitionStyle) const
{
    return settings.value("slideshow/transitionStyle", defaultTransitionStyle).toInt();
}

void SnapflowSettings::setSlideshowTransitionStyle(int transitionStyle)
{
    settings.setValue("slideshow/transitionStyle", transitionStyle);
}

int SnapflowSettings::slideshowTransitionSoftness(int defaultTransitionStyle) const
{
    return settings.value("slideshow/transitionSoftness", defaultTransitionStyle).toInt();
}

void SnapflowSettings::setSlideshowTransitionSoftness(int transitionSoftness)
{
    settings.setValue("slideshow/transitionSoftness", transitionSoftness);
}

/*!
    \qmlproperty bool Settings::keyframesDragScrub
    \brief Whether scrubbing occurs while dragging keyframes.
*/

bool SnapflowSettings::keyframesDragScrub() const
{
    return settings.value("keyframes/dragScrub", false).toBool();
}

void SnapflowSettings::setKeyframesDragScrub(bool b)
{
    settings.setValue("keyframes/dragScrub", b);
    emit keyframesDragScrubChanged();
}

void SnapflowSettings::setSubtitlesShowColumn(const QString &column, bool b)
{
    settings.setValue("subtitles/columns/" + column, b);
}

bool SnapflowSettings::subtitlesShowColumn(const QString &column) const
{
    return settings.value("subtitles/columns/" + column, true).toBool();
}

void SnapflowSettings::setSubtitlesTrackTimeline(bool b)
{
    settings.setValue("subtitles/trackTimeline", b);
}

bool SnapflowSettings::subtitlesTrackTimeline() const
{
    return settings.value("subtitles/trackTimeline", true).toBool();
}

void SnapflowSettings::setSubtitlesShowPrevNext(bool b)
{
    settings.setValue("subtitles/showPrevNext", b);
}

bool SnapflowSettings::subtitlesShowPrevNext() const
{
    return settings.value("subtitles/showPrevNext", true).toBool();
}

QString SnapflowSettings::speechLanguage() const
{
    return settings.value("speech/language", QStringLiteral("a")).toString();
}

void SnapflowSettings::setSpeechLanguage(const QString &code)
{
    settings.setValue("speech/language", code);
}

QString SnapflowSettings::speechVoice() const
{
    return settings.value("speech/voice", QString()).toString();
}

void SnapflowSettings::setSpeechVoice(const QString &voiceId)
{
    settings.setValue("speech/voice", voiceId);
}

double SnapflowSettings::speechSpeed() const
{
    return settings.value("speech/speed", 1.0).toDouble();
}

void SnapflowSettings::setSpeechSpeed(double speed)
{
    settings.setValue("speech/speed", speed);
}

void SnapflowSettings::saveCustomColors()
{
    // QColorDialog supports up to 48 custom colors (16 in older versions)
    QStringList colorList;
    for (int i = 0; i < QColorDialog::customCount(); ++i) {
        QColor color = QColorDialog::customColor(i);
        if (color.isValid()) {
            colorList.append(color.name(QColor::HexArgb));
        } else {
            colorList.append(QString());
        }
    }
    settings.setValue("colorDialog/customColors", colorList);
}

void SnapflowSettings::restoreCustomColors()
{
    QStringList colorList = settings.value("colorDialog/customColors").toStringList();
    for (int i = 0; i < colorList.size() && i < QColorDialog::customCount(); ++i) {
        const QString &colorName = colorList.at(i);
        if (!colorName.isEmpty()) {
            QColor color(colorName);
            if (color.isValid()) {
                // Use rgba() to preserve alpha channel
                QColorDialog::setCustomColor(i, color.rgba());
            }
        }
    }
}

void SnapflowSettings::setWhisperExe(const QString &path)
{
    settings.setValue("subtitles/whisperExe", path);
}

QString SnapflowSettings::whisperExe()
{
    QDir dir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
    auto exe = "whisper-cli.exe";
#else
    auto exe = "whisper-cli";
#endif
    return settings.value("subtitles/whisperExe", dir.absoluteFilePath(exe)).toString();
}

void SnapflowSettings::setWhisperModel(const QString &path)
{
    settings.setValue("subtitles/whisperModel", path);
}

QString SnapflowSettings::whisperModel()
{
    QDir dataPath = QmlApplication::dataDir();
    dataPath.cd("snapflow/whisper_models");
    return settings.value("subtitles/whisperModel", "").toString();
}

void SnapflowSettings::setWhisperUseGpu(bool b)
{
    settings.setValue("subtitles/whisperUseGpu", b);
}

bool SnapflowSettings::whisperUseGpu() const
{
    return settings.value("subtitles/whisperUseGpu", true).toBool();
}

void SnapflowSettings::setNotesZoom(int zoom)
{
    settings.setValue("notes/zoom", zoom);
}

int SnapflowSettings::notesZoom() const
{
    return settings.value("notes/zoom", 0).toInt();
}

void SnapflowSettings::reset()
{
    for (auto &key : settings.allKeys()) {
        settings.remove(key);
    }
}

int SnapflowSettings::undoLimit() const
{
    return settings.value("undoLimit", 50).toInt();
}

bool SnapflowSettings::warnLowMemory() const
{
    return settings.value("warnLowMemory", true).toBool();
}

int SnapflowSettings::backupPeriod() const
{
    return settings.value("backupPeriod", 24 * 60).toInt();
}

void SnapflowSettings::setBackupPeriod(int minutes)
{
    settings.setValue("backupPeriod", minutes);
}

QDateTime SnapflowSettings::lastBackupDateTime(const QString &filePath) const
{
    return settings.value("lastBackupDateTimeMap").toMap().value(filePath).toDateTime();
}

void SnapflowSettings::setLastBackupDateTime(const QString &filePath, const QDateTime &dt)
{
    static const int kMaxBackupEntries = 100;
    auto map = settings.value("lastBackupDateTimeMap").toMap();
    if (dt.isValid())
        map[filePath] = dt;
    else
        map.remove(filePath);
    // Prune entries for files that no longer exist.
    for (const auto &path : map.keys())
        if (!QFile::exists(path))
            map.remove(path);
    // If still over the limit, remove the oldest entries.
    while (map.size() > kMaxBackupEntries) {
        map.erase(std::min_element(map.begin(), map.end(), [](const QVariant &a, const QVariant &b) {
            return a.toDateTime() < b.toDateTime();
        }));
    }
    settings.setValue("lastBackupDateTimeMap", map);
}

mlt_time_format SnapflowSettings::timeFormat() const
{
    return (mlt_time_format) settings.value("timeFormat", mlt_time_clock).toInt();
}

void SnapflowSettings::setTimeFormat(int format)
{
    settings.setValue("timeFormat", format);
    emit timeFormatChanged();
}

bool SnapflowSettings::askFlatpakWrappers()
{
    return settings.value("flatpakWrappers", true).toBool();
}

void SnapflowSettings::setAskFlatpakWrappers(bool b)
{
    settings.setValue("flatpakWrappers", b);
}

QString SnapflowSettings::dockerPath() const
{
#if defined(Q_OS_MAC)
    return settings.value("dockerPath", "/usr/local/bin/docker").toString();
#elif defined(Q_OS_WIN)
    return settings.value("dockerPath", "C:/Program Files/Docker/Docker/resources/bin/docker.exe")
        .toString();
#else
    return settings.value("dockerPath", "docker").toString();
#endif
}

void SnapflowSettings::setDockerPath(const QString &path)
{
    settings.setValue("dockerPath", path);
}

QString SnapflowSettings::chromiumPath() const
{
#if defined(Q_OS_MAC)
    return settings.value("chromiumPath", "/Applications/Google Chrome.app").toString();
#elif defined(Q_OS_WIN)
    return settings.value("chromiumPath", "C:/Program Files/Google/Chrome/Application/chrome.exe")
        .toString();
#else
    return settings.value("chromiumPath", "/usr/bin/chromium-browser").toString();
#endif
}

void SnapflowSettings::setChromiumPath(const QString &path)
{
    settings.setValue("chromiumPath", path);
}

QString SnapflowSettings::screenRecorderPath() const
{
    return settings.value("screenRecorderPath", "obs").toString();
}

void SnapflowSettings::setScreenRecorderPath(const QString &path)
{
    settings.setValue("screenRecorderPath", path);
}
