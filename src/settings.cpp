/*
 * Copyright (c) 2013-2025 Meltytech, LLC
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
static const QString SHOTCUT_INI_FILENAME("/shotcut.ini");
static const QString RECENT_INI_FILENAME("recent.ini");
static QScopedPointer<ShotcutSettings> instance;
static QString appDataForSession;
static const int kMaximumTrackHeight = 125;
static const QString kRecentKey("recent");
static const QString kProjectsKey("projects");

namespace {
struct ModeMap
{
    ShotcutSettings::ProcessingMode id;
    const char *name;
};
static const ModeMap kModeMap[] = {
    {ShotcutSettings::Native8Cpu, "Native8Cpu"},
    {ShotcutSettings::Linear8Cpu, "Linear8Cpu"},
    {ShotcutSettings::Native10Cpu, "Native10Cpu"},
    {ShotcutSettings::Linear10Cpu, "Linear10Cpu"},
    {ShotcutSettings::Linear10GpuCpu, "Linear10GpuCpu"},
};
} // anonymous namespace

ShotcutSettings &ShotcutSettings::singleton()
{
    if (!instance) {
        if (appDataForSession.isEmpty()) {
            instance.reset(new ShotcutSettings);
            if (instance->settings.value(APP_DATA_DIR_KEY).isValid()
                && QFile::exists(instance->settings.value(APP_DATA_DIR_KEY).toString()
                                 + SHOTCUT_INI_FILENAME))
                instance.reset(
                    new ShotcutSettings(instance->settings.value(APP_DATA_DIR_KEY).toString()));
        } else {
            instance.reset(new ShotcutSettings(appDataForSession));
        }
    }
    return *instance;
}

ShotcutSettings::ShotcutSettings()
    : QObject()
    , m_recent(QDir(appDataLocation()).filePath(RECENT_INI_FILENAME), QSettings::IniFormat)
{
    migrateLayout();
    migrateRecent();
}

ShotcutSettings::ShotcutSettings(const QString &appDataLocation)
    : QObject()
    , settings(appDataLocation + SHOTCUT_INI_FILENAME, QSettings::IniFormat)
    , m_appDataLocation(appDataLocation)
    , m_recent(QDir(appDataLocation).filePath(RECENT_INI_FILENAME), QSettings::IniFormat)
{
    migrateLayout();
    migrateRecent();
}

void ShotcutSettings::migrateRecent()
{
    // Migrate recent to separate INI file
    auto oldRecents = settings.value(kRecentKey).toStringList();
    if (recent().isEmpty() && !oldRecents.isEmpty()) {
        auto newRecents = recent();
        for (const auto &a : oldRecents) {
            if (a.size() < ShotcutSettings::MaxPath && !newRecents.contains(a)) {
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

void ShotcutSettings::migrateLayout()
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

void ShotcutSettings::log()
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

QString ShotcutSettings::language() const
{
    QString language = settings.value("language", QLocale().name()).toString();
    if (language == "en")
        language = "en_US";
    return language;
}

void ShotcutSettings::setLanguage(const QString &s)
{
    settings.setValue("language", s);
}

double ShotcutSettings::imageDuration() const
{
    return settings.value("imageDuration", 4.0).toDouble();
}

void ShotcutSettings::setImageDuration(double d)
{
    settings.setValue("imageDuration", d);
}

QString ShotcutSettings::openPath() const
{
    return settings
        .value("openPath", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation))
        .toString();
}

void ShotcutSettings::setOpenPath(const QString &s)
{
    settings.setValue("openPath", s);
    emit savePathChanged();
}

QString ShotcutSettings::savePath() const
{
    return settings
        .value("savePath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation))
        .toString();
}

void ShotcutSettings::setSavePath(const QString &s)
{
    settings.setValue("savePath", s);
    emit savePathChanged();
}

QStringList ShotcutSettings::recent() const
{
    return m_recent.value(kRecentKey).toStringList();
}

void ShotcutSettings::setRecent(const QStringList &ls)
{
    if (ls.isEmpty())
        m_recent.remove(kRecentKey);
    else if (!clearRecent())
        m_recent.setValue(kRecentKey, ls);
}

QStringList ShotcutSettings::projects()
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

void ShotcutSettings::setProjects(const QStringList &ls)
{
    if (ls.isEmpty())
        m_recent.remove(kProjectsKey);
    else if (!clearRecent())
        m_recent.setValue(kProjectsKey, ls);
}

QString ShotcutSettings::theme() const
{
    return settings.value("theme", "dark").toString();
}

void ShotcutSettings::setTheme(const QString &s)
{
    settings.setValue("theme", s);
}

QThread::Priority ShotcutSettings::jobPriority() const
{
    const auto priority = settings.value("jobPriority", "low").toString();
    if (priority == "low") {
        return QThread::LowPriority;
    }
    return QThread::NormalPriority;
}

void ShotcutSettings::setJobPriority(const QString &s)
{
    settings.setValue("jobPriority", s);
}

bool ShotcutSettings::showTitleBars() const
{
    return settings.value("titleBars", true).toBool();
}

void ShotcutSettings::setShowTitleBars(bool b)
{
    settings.setValue("titleBars", b);
}

bool ShotcutSettings::showToolBar() const
{
    return settings.value("toolBar", true).toBool();
}

void ShotcutSettings::setShowToolBar(bool b)
{
    settings.setValue("toolBar", b);
}

bool ShotcutSettings::textUnderIcons() const
{
    return settings.value("textUnderIcons", true).toBool();
}

void ShotcutSettings::setTextUnderIcons(bool b)
{
    settings.setValue("textUnderIcons", b);
}

bool ShotcutSettings::smallIcons() const
{
    return settings.value("smallIcons", false).toBool();
}

void ShotcutSettings::setSmallIcons(bool b)
{
    settings.setValue("smallIcons", b);
    emit smallIconsChanged();
}

QByteArray ShotcutSettings::windowGeometry() const
{
    return settings.value("geometry2").toByteArray();
}

void ShotcutSettings::setWindowGeometry(const QByteArray &a)
{
    settings.setValue("geometry2", a);
}

QByteArray ShotcutSettings::windowGeometryDefault() const
{
    return settings.value("geometryDefault").toByteArray();
}

void ShotcutSettings::setWindowGeometryDefault(const QByteArray &a)
{
    settings.setValue("geometryDefault", a);
}

QByteArray ShotcutSettings::windowState() const
{
    return settings.value("windowState2").toByteArray();
}

void ShotcutSettings::setWindowState(const QByteArray &a)
{
    settings.setValue("windowState2", a);
}

QByteArray ShotcutSettings::windowStateDefault() const
{
    return settings.value("windowStateDefault").toByteArray();
}

void ShotcutSettings::setWindowStateDefault(const QByteArray &a)
{
    settings.setValue("windowStateDefault", a);
}

QString ShotcutSettings::viewMode() const
{
    return settings.value("playlist/viewMode").toString();
}

void ShotcutSettings::setViewMode(const QString &viewMode)
{
    settings.setValue("playlist/viewMode", viewMode);
    emit viewModeChanged();
}

QString ShotcutSettings::filesViewMode() const
{
    return settings.value("files/viewMode", QLatin1String("tiled")).toString();
}

void ShotcutSettings::setFilesViewMode(const QString &viewMode)
{
    settings.setValue("files/viewMode", viewMode);
    emit filesViewModeChanged();
}

QStringList ShotcutSettings::filesLocations() const
{
    QStringList result;
    for (const auto &s : settings.value("files/locations").toStringList()) {
        if (!s.startsWith("__"))
            result << s;
    }
    return result;
}

QString ShotcutSettings::filesLocationPath(const QString &name) const
{
    QString key = QStringLiteral("files/location/%1").arg(name);
    return settings.value(key).toString();
}

bool ShotcutSettings::setFilesLocation(const QString &name, const QString &path)
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

bool ShotcutSettings::removeFilesLocation(const QString &name)
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

QStringList ShotcutSettings::filesOpenOther(const QString &type) const
{
    return settings.value("files/openOther/" + type).toStringList();
}

void ShotcutSettings::setFilesOpenOther(const QString &type, const QString &filePath)
{
    QStringList filePaths = filesOpenOther(type);
    filePaths.removeAll(filePath);
    filePaths.append(filePath);
    settings.setValue("files/openOther/" + type, filePaths);
}

bool ShotcutSettings::removeFilesOpenOther(const QString &type, const QString &filePath)
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

QString ShotcutSettings::filesCurrentDir() const
{
    const auto ls = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    auto path = settings.value("files/currentDir", ls.first()).toString();
    if (!QFile::exists(path)) {
        LOG_DEBUG() << "dir does not exist:" << QDir::toNativeSeparators(path);
        path = ls.first();
    }
    return path;
}

void ShotcutSettings::setFilesCurrentDir(const QString &s)
{
    settings.setValue("files/currentDir", s);
}

bool ShotcutSettings::filesFoldersOpen() const
{
    return settings.value("files/foldersOpen", true).toBool();
}

void ShotcutSettings::setFilesFoldersOpen(bool b)
{
    settings.setValue("files/foldersOpen", b);
}

QString ShotcutSettings::exportFrameSuffix() const
{
    return settings.value("exportFrameSuffix", ".png").toString();
}

void ShotcutSettings::setExportFrameSuffix(const QString &exportFrameSuffix)
{
    settings.setValue("exportFrameSuffix", exportFrameSuffix);
}

QString ShotcutSettings::encodePath() const
{
    return settings
        .value("encode/path", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation))
        .toString();
}

void ShotcutSettings::setEncodePath(const QString &s)
{
    settings.setValue("encode/path", s);
}

bool ShotcutSettings::encodeFreeSpaceCheck() const
{
    return settings.value("encode/freeSpaceCheck", true).toBool();
}

void ShotcutSettings::setEncodeFreeSpaceCheck(bool b)
{
    settings.setValue("encode/freeSpaceCheck", b);
}

bool ShotcutSettings::encodeUseHardware() const
{
    return settings.value("encode/useHardware").toBool();
}

void ShotcutSettings::setEncodeUseHardware(bool b)
{
    settings.setValue("encode/useHardware", b);
}

QStringList ShotcutSettings::encodeHardware() const
{
    return settings.value("encode/hardware").toStringList();
}

void ShotcutSettings::setEncodeHardware(const QStringList &ls)
{
    if (ls.isEmpty())
        settings.remove("encode/hardware");
    else
        settings.setValue("encode/hardware", ls);
}

bool ShotcutSettings::encodeAdvanced() const
{
    return settings.value("encode/advanced", false).toBool();
}

void ShotcutSettings::setEncodeAdvanced(bool b)
{
    settings.setValue("encode/advanced", b);
}

bool ShotcutSettings::convertAdvanced() const
{
    return settings.value("convertAdvanced", false).toBool();
}

void ShotcutSettings::setConvertAdvanced(bool b)
{
    settings.setValue("convertAdvanced", b);
}

ShotcutSettings::ProcessingMode ShotcutSettings::processingMode()
{
    if (settings.contains("processingMode")) {
        auto result = (ShotcutSettings::ProcessingMode) settings.value("processingMode").toInt();
        if (result == Linear8Cpu) {
            // No longer supported but kept to prevent unexpected processing behavior going from
            // beta to release
            result = Native8Cpu;
        }
        return result;
    } else if (settings.contains("player/gpu2")) {
        // Legacy GPU Mode
        if (settings.value("player/gpu2").toBool()) {
            return ShotcutSettings::Linear10GpuCpu;
        }
    }
    return ShotcutSettings::Native8Cpu;
}

void ShotcutSettings::setProcessingMode(ProcessingMode mode)
{
    settings.setValue("processingMode", mode);
    emit playerGpuChanged();
}

QString ShotcutSettings::processingModeStr(ShotcutSettings::ProcessingMode mode)
{
    for (const auto &m : kModeMap) {
        if (m.id == mode)
            return QString::fromLatin1(m.name);
    }
    LOG_ERROR() << "Unknown processing mode" << mode;
    return QStringLiteral("Native8Cpu");
}

ShotcutSettings::ProcessingMode ShotcutSettings::processingModeId(const QString &mode)
{
    for (const auto &m : kModeMap) {
        if (mode == QLatin1String(m.name))
            return m.id;
    }
    LOG_ERROR() << "Unknown processing mode" << mode;
    return Native8Cpu;
}

bool ShotcutSettings::showConvertClipDialog() const
{
    return settings.value("showConvertClipDialog", true).toBool();
}

void ShotcutSettings::setShowConvertClipDialog(bool b)
{
    settings.setValue("showConvertClipDialog", b);
}

bool ShotcutSettings::encodeParallelProcessing() const
{
    return settings.value("encode/parallelProcessing", false).toBool();
}

void ShotcutSettings::setEncodeParallelProcessing(bool b)
{
    settings.setValue("encode/parallelProcessing", b);
}

int ShotcutSettings::playerAudioChannels() const
{
    return settings.value("player/audioChannels", 2).toInt();
}

void ShotcutSettings::setPlayerAudioChannels(int i)
{
    settings.setValue("player/audioChannels", i);
    emit playerAudioChannelsChanged(i);
}

QString ShotcutSettings::playerDeinterlacer() const
{
    QString result = settings.value("player/deinterlacer", "onefield").toString();
    //XXX workaround yadif crashing with mlt_transition
    if (result == "yadif" || result == "yadif-nospatial")
        result = "onefield";
    return result;
}

void ShotcutSettings::setPlayerDeinterlacer(const QString &s)
{
    settings.setValue("player/deinterlacer", s);
}

QString ShotcutSettings::playerExternal() const
{
    auto result = settings.value("player/external", "").toString();
    // "sdi" is no longer supported DVEO VidPort
    return result == "sdi" ? "" : result;
}

void ShotcutSettings::setPlayerExternal(const QString &s)
{
    settings.setValue("player/external", s);
}

bool ShotcutSettings::playerJACK() const
{
    return settings.value("player/jack", false).toBool();
}

QString ShotcutSettings::playerInterpolation() const
{
    return settings.value("player/interpolation", "bilinear").toString();
}

void ShotcutSettings::setPlayerInterpolation(const QString &s)
{
    settings.setValue("player/interpolation", s);
}

bool ShotcutSettings::playerGPU() const
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

bool ShotcutSettings::playerWarnGPU() const
{
    return false; //settings.value("player/warnGPU", false).toBool();
}

void ShotcutSettings::setPlayerJACK(bool b)
{
    settings.setValue("player/jack", b);
}

int ShotcutSettings::playerDecklinkGamma() const
{
    return settings.value("player/decklinkGamma", 0).toInt();
}

void ShotcutSettings::setPlayerDecklinkGamma(int i)
{
    settings.setValue("player/decklinkGamma", i);
}

int ShotcutSettings::playerKeyerMode() const
{
    return settings.value("player/keyer", 0).toInt();
}

void ShotcutSettings::setPlayerKeyerMode(int i)
{
    settings.setValue("player/keyer", i);
}

bool ShotcutSettings::playerMuted() const
{
    return settings.value("player/muted", false).toBool();
}

void ShotcutSettings::setPlayerMuted(bool b)
{
    settings.setValue("player/muted", b);
}

QString ShotcutSettings::playerProfile() const
{
    return settings.value("player/profile", "").toString();
}

void ShotcutSettings::setPlayerProfile(const QString &s)
{
    settings.setValue("player/profile", s);
}

bool ShotcutSettings::playerProgressive() const
{
    return settings.value("player/progressive", true).toBool();
}

void ShotcutSettings::setPlayerProgressive(bool b)
{
    settings.setValue("player/progressive", b);
}

bool ShotcutSettings::playerRealtime() const
{
    return settings.value("player/realtime", true).toBool();
}

void ShotcutSettings::setPlayerRealtime(bool b)
{
    settings.setValue("player/realtime", b);
}

bool ShotcutSettings::playerScrubAudio() const
{
    return settings.value("player/scrubAudio", true).toBool();
}

void ShotcutSettings::setPlayerScrubAudio(bool b)
{
    settings.setValue("player/scrubAudio", b);
}

int ShotcutSettings::playerVolume() const
{
    return settings.value("player/volume", 88).toInt();
}

void ShotcutSettings::setPlayerVolume(int i)
{
    settings.setValue("player/volume", i);
}

float ShotcutSettings::playerZoom() const
{
    return settings.value("player/zoom", 0.0f).toFloat();
}

void ShotcutSettings::setPlayerZoom(float f)
{
    settings.setValue("player/zoom", f);
}

int ShotcutSettings::playerPreviewScale() const
{
    return settings.value("player/previewScale", 0).toInt();
}

void ShotcutSettings::setPlayerPreviewScale(int i)
{
    settings.setValue("player/previewScale", i);
}

int ShotcutSettings::playerVideoDelayMs() const
{
    return settings.value("player/videoDelayMs", 0).toInt();
}

void ShotcutSettings::setPlayerVideoDelayMs(int i)
{
    settings.setValue("player/videoDelayMs", i);
}

double ShotcutSettings::playerJumpSeconds() const
{
    return settings.value("player/jumpSeconds", 60.0).toDouble();
}

void ShotcutSettings::setPlayerJumpSeconds(double i)
{
    settings.setValue("player/jumpSeconds", i);
}

QString ShotcutSettings::playerAudioDriver() const
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

void ShotcutSettings::setPlayerAudioDriver(const QString &s)
{
    settings.setValue("player/audioDriver", s);
}

bool ShotcutSettings::playerPauseAfterSeek() const
{
    return settings.value("player/pauseAfterSeek", true).toBool();
}

void ShotcutSettings::setPlayerPauseAfterSeek(bool b)
{
    settings.setValue("player/pauseAfterSeek", b);
}

QString ShotcutSettings::playlistThumbnails() const
{
    return settings.value("playlist/thumbnails", "small").toString();
}

void ShotcutSettings::setPlaylistThumbnails(const QString &s)
{
    settings.setValue("playlist/thumbnails", s);
    emit playlistThumbnailsChanged();
}

bool ShotcutSettings::playlistAutoplay() const
{
    return settings.value("playlist/autoplay", true).toBool();
}

void ShotcutSettings::setPlaylistAutoplay(bool b)
{
    settings.setValue("playlist/autoplay", b);
}

bool ShotcutSettings::playlistShowColumn(const QString &column)
{
    return settings.value("playlist/columns/" + column, true).toBool();
}

void ShotcutSettings::setPlaylistShowColumn(const QString &column, bool b)
{
    settings.setValue("playlist/columns/" + column, b);
}

bool ShotcutSettings::timelineDragScrub() const
{
    return settings.value("timeline/dragScrub", false).toBool();
}

void ShotcutSettings::setTimelineDragScrub(bool b)
{
    settings.setValue("timeline/dragScrub", b);
    emit timelineDragScrubChanged();
}

bool ShotcutSettings::timelineShowWaveforms() const
{
    return settings.value("timeline/waveforms", true).toBool();
}

void ShotcutSettings::setTimelineShowWaveforms(bool b)
{
    settings.setValue("timeline/waveforms", b);
    emit timelineShowWaveformsChanged();
}

bool ShotcutSettings::timelineShowThumbnails() const
{
    return settings.value("timeline/thumbnails", true).toBool();
}

void ShotcutSettings::setTimelineShowThumbnails(bool b)
{
    settings.setValue("timeline/thumbnails", b);
    emit timelineShowThumbnailsChanged();
}

bool ShotcutSettings::timelineRipple() const
{
    return settings.value("timeline/ripple", false).toBool();
}

void ShotcutSettings::setTimelineRipple(bool b)
{
    settings.setValue("timeline/ripple", b);
    emit timelineRippleChanged();
}

bool ShotcutSettings::timelineRippleAllTracks() const
{
    return settings.value("timeline/rippleAllTracks", false).toBool();
}

void ShotcutSettings::setTimelineRippleAllTracks(bool b)
{
    settings.setValue("timeline/rippleAllTracks", b);
    emit timelineRippleAllTracksChanged();
}

bool ShotcutSettings::timelineRippleMarkers() const
{
    return settings.value("timeline/rippleMarkers", false).toBool();
}

void ShotcutSettings::setTimelineRippleMarkers(bool b)
{
    settings.setValue("timeline/rippleMarkers", b);
    emit timelineRippleMarkersChanged();
}

bool ShotcutSettings::timelineSnap() const
{
    return settings.value("timeline/snap", true).toBool();
}

void ShotcutSettings::setTimelineSnap(bool b)
{
    settings.setValue("timeline/snap", b);
    emit timelineSnapChanged();
}

int ShotcutSettings::timelineTrackHeight() const
{
    return qMin(settings.value("timeline/trackHeight", 50).toInt(), kMaximumTrackHeight);
}

void ShotcutSettings::setTimelineTrackHeight(int n)
{
    settings.setValue("timeline/trackHeight", qMin(n, kMaximumTrackHeight));
}

bool ShotcutSettings::timelineScrollZoom() const
{
    return settings.value("timeline/scrollZoom", true).toBool();
}

void ShotcutSettings::setTimelineScrollZoom(bool b)
{
    settings.setValue("timeline/scrollZoom", b);
    emit timelineScrollZoomChanged();
}

bool ShotcutSettings::timelineFramebufferWaveform() const
{
    return settings.value("timeline/framebufferWaveform", true).toBool();
}

void ShotcutSettings::setTimelineFramebufferWaveform(bool b)
{
    settings.setValue("timeline/framebufferWaveform", b);
    emit timelineFramebufferWaveformChanged();
}

int ShotcutSettings::audioReferenceTrack() const
{
    return settings.value("timeline/audioReferenceTrack", 0).toInt();
}
void ShotcutSettings::setAudioReferenceTrack(int track)
{
    settings.setValue("timeline/audioReferenceTrack", track);
}

double ShotcutSettings::audioReferenceSpeedRange() const
{
    return settings.value("timeline/audioReferenceSpeedRange", 0).toDouble();
}
void ShotcutSettings::setAudioReferenceSpeedRange(double range)
{
    settings.setValue("timeline/audioReferenceSpeedRange", range);
}

bool ShotcutSettings::timelinePreviewTransition() const
{
    return settings.value("timeline/previewTransition", true).toBool();
}

void ShotcutSettings::setTimelinePreviewTransition(bool b)
{
    settings.setValue("timeline/previewTransition", b);
}

void ShotcutSettings::setTimelineScrolling(ShotcutSettings::TimelineScrolling value)
{
    settings.remove("timeline/centerPlayhead");
    settings.setValue("timeline/scrolling", value);
    emit timelineScrollingChanged();
}

ShotcutSettings::TimelineScrolling ShotcutSettings::timelineScrolling() const
{
    if (settings.contains("timeline/centerPlayhead")
        && settings.value("timeline/centerPlayhead").toBool())
        return ShotcutSettings::TimelineScrolling::CenterPlayhead;
    else
        return ShotcutSettings::TimelineScrolling(
            settings.value("timeline/scrolling", PageScrolling).toInt());
}

bool ShotcutSettings::timelineAutoAddTracks() const
{
    return settings.value("timeline/autoAddTracks", true).toBool();
}

void ShotcutSettings::setTimelineAutoAddTracks(bool b)
{
    if (b != timelineAutoAddTracks()) {
        settings.setValue("timeline/autoAddTracks", b);
        emit timelineAutoAddTracksChanged();
    }
}

bool ShotcutSettings::timelineRectangleSelect() const
{
    return settings.value("timeline/rectangleSelect", true).toBool();
}

void ShotcutSettings::setTimelineRectangleSelect(bool b)
{
    settings.setValue("timeline/rectangleSelect", b);
    emit timelineRectangleSelectChanged();
}

bool ShotcutSettings::timelineAdjustGain() const
{
    return settings.value("timeline/adjustGain", false).toBool();
}

void ShotcutSettings::setTimelineAdjustGain(bool b)
{
    settings.setValue("timeline/adjustGain", b);
    emit timelineAdjustGainChanged();
}

QString ShotcutSettings::filterFavorite(const QString &filterName)
{
    return settings.value("filter/favorite/" + filterName, "").toString();
}
void ShotcutSettings::setFilterFavorite(const QString &filterName, const QString &value)
{
    settings.setValue("filter/favorite/" + filterName, value);
}

double ShotcutSettings::audioInDuration() const
{
    return settings.value("filter/audioInDuration", 1.0).toDouble();
}

void ShotcutSettings::setAudioInDuration(double d)
{
    settings.setValue("filter/audioInDuration", d);
    emit audioInDurationChanged();
}

double ShotcutSettings::audioOutDuration() const
{
    return settings.value("filter/audioOutDuration", 1.0).toDouble();
}

void ShotcutSettings::setAudioOutDuration(double d)
{
    settings.setValue("filter/audioOutDuration", d);
    emit audioOutDurationChanged();
}

double ShotcutSettings::videoInDuration() const
{
    return settings.value("filter/videoInDuration", 1.0).toDouble();
}

void ShotcutSettings::setVideoInDuration(double d)
{
    settings.setValue("filter/videoInDuration", d);
    emit videoInDurationChanged();
}

double ShotcutSettings::videoOutDuration() const
{
    return settings.value("filter/videoOutDuration", 1.0).toDouble();
}

void ShotcutSettings::setVideoOutDuration(double d)
{
    settings.setValue("filter/videoOutDuration", d);
    emit videoOutDurationChanged();
}

int ShotcutSettings::audioInCurve() const
{
    return settings.value("filter/audioInCurve", mlt_keyframe_linear).toInt();
}

void ShotcutSettings::setAudioInCurve(int c)
{
    settings.setValue("filter/audioInCurve", c);
    emit audioInCurveChanged();
}

int ShotcutSettings::audioOutCurve() const
{
    return settings.value("filter/audioOutCurve", mlt_keyframe_linear).toInt();
}

void ShotcutSettings::setAudioOutCurve(int c)
{
    settings.setValue("filter/audioOutCurve", c);
    emit audioOutCurveChanged();
}

bool ShotcutSettings::askOutputFilter() const
{
    return settings.value("filter/askOutput", true).toBool();
}

void ShotcutSettings::setAskOutputFilter(bool b)
{
    settings.setValue("filter/askOutput", b);
    emit askOutputFilterChanged();
}

bool ShotcutSettings::loudnessScopeShowMeter(const QString &meter) const
{
    return settings.value("scope/loudness/" + meter, true).toBool();
}

void ShotcutSettings::setLoudnessScopeShowMeter(const QString &meter, bool b)
{
    settings.setValue("scope/loudness/" + meter, b);
}

void ShotcutSettings::setMarkerColor(const QColor &color)
{
    settings.setValue("markers/color", color.name());
}

QColor ShotcutSettings::markerColor() const
{
    return QColor(settings.value("markers/color", "green").toString());
}

void ShotcutSettings::setMarkersShowColumn(const QString &column, bool b)
{
    settings.setValue("markers/columns/" + column, b);
}

bool ShotcutSettings::markersShowColumn(const QString &column) const
{
    return settings.value("markers/columns/" + column, true).toBool();
}

void ShotcutSettings::setMarkerSort(int column, Qt::SortOrder order)
{
    settings.setValue("markers/sortColumn", column);
    settings.setValue("markers/sortOrder", order);
}

int ShotcutSettings::getMarkerSortColumn()
{
    return settings.value("markers/sortColumn", -1).toInt();
}

Qt::SortOrder ShotcutSettings::getMarkerSortOrder()
{
    return (Qt::SortOrder) settings.value("markers/sortOrder", Qt::AscendingOrder).toInt();
}

int ShotcutSettings::drawMethod() const
{
#ifdef Q_OS_WIN
    return settings.value("opengl", Qt::AA_UseOpenGLES).toInt();
#else
    return settings.value("opengl", Qt::AA_UseDesktopOpenGL).toInt();
#endif
}

void ShotcutSettings::setDrawMethod(int i)
{
    settings.setValue("opengl", i);
}

bool ShotcutSettings::noUpgrade() const
{
    return settings.value("noupgrade", false).toBool();
}

void ShotcutSettings::setNoUpgrade(bool value)
{
    settings.setValue("noupgrade", value);
}

bool ShotcutSettings::checkUpgradeAutomatic()
{
    return settings.value("checkUpgradeAutomatic", false).toBool();
}

void ShotcutSettings::setCheckUpgradeAutomatic(bool b)
{
    settings.setValue("checkUpgradeAutomatic", b);
}

bool ShotcutSettings::askUpgradeAutomatic()
{
    return settings.value("askUpgradeAutmatic", true).toBool();
}

void ShotcutSettings::setAskUpgradeAutomatic(bool b)
{
    settings.setValue("askUpgradeAutmatic", b);
}

bool ShotcutSettings::askChangeVideoMode()
{
    return settings.value("askChangeVideoMode", true).toBool();
}

void ShotcutSettings::setAskChangeVideoMode(bool b)
{
    settings.setValue("askChangeVideoMode", b);
}

void ShotcutSettings::sync()
{
    settings.sync();
}

QString ShotcutSettings::appDataLocation() const
{
    if (!m_appDataLocation.isEmpty())
        return m_appDataLocation;
    else
        return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

void ShotcutSettings::setAppDataForSession(const QString &location)
{
    // This is intended to be called when using a command line option
    // to set the AppData location.
    appDataForSession = location;
    if (instance)
        instance.reset(new ShotcutSettings(location));
}

void ShotcutSettings::setAppDataLocally(const QString &location)
{
    // This is intended to be called when using a GUI action to set the
    // the new AppData location.

    // Copy the existing settings if they exist.
    if (!QFile::exists(location + SHOTCUT_INI_FILENAME)) {
        QSettings newSettings(location + SHOTCUT_INI_FILENAME, QSettings::IniFormat);
        foreach (const QString &key, settings.allKeys())
            newSettings.setValue(key, settings.value(key));
        newSettings.sync();
    }

    // Set the new location.
    QSettings localSettings;
    localSettings.setValue(APP_DATA_DIR_KEY, location);
    localSettings.sync();
}

QStringList ShotcutSettings::layouts() const
{
    QStringList result;
    for (const auto &s : settings.value("layout/layouts").toStringList()) {
        if (!s.startsWith("__"))
            result << s;
    }
    return result;
}

bool ShotcutSettings::setLayout(const QString &name,
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

QByteArray ShotcutSettings::layoutGeometry(const QString &name)
{
    QString key = QStringLiteral("layout/%1_geometry").arg(name);
    return settings.value(key).toByteArray();
}

QByteArray ShotcutSettings::layoutState(const QString &name)
{
    QString key = QStringLiteral("layout/%1_state").arg(name);
    return settings.value(key).toByteArray();
}

bool ShotcutSettings::removeLayout(const QString &name)
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

int ShotcutSettings::layoutMode() const
{
    return settings.value("layout/mode", -1).toInt();
}

void ShotcutSettings::setLayoutMode(int mode)
{
    settings.setValue("layout/mode", mode);
}

bool ShotcutSettings::clearRecent() const
{
    return settings.value("clearRecent", false).toBool();
}

void ShotcutSettings::setClearRecent(bool b)
{
    settings.setValue("clearRecent", b);
}

QString ShotcutSettings::projectsFolder() const
{
    return settings
        .value("projectsFolder", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation))
        .toString();
}

void ShotcutSettings::setProjectsFolder(const QString &path)
{
    settings.setValue("projectsFolder", path);
}

QString ShotcutSettings::audioInput() const
{
    QString defaultValue = "default";
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    for (const auto &deviceInfo : QMediaDevices::audioInputs()) {
        defaultValue = deviceInfo.description();
    }
#endif
    return settings.value("audioInput", defaultValue).toString();
}

void ShotcutSettings::setAudioInput(const QString &name)
{
    settings.setValue("audioInput", name);
}

QString ShotcutSettings::videoInput() const
{
    return settings.value("videoInput").toString();
}

void ShotcutSettings::setVideoInput(const QString &name)
{
    settings.setValue("videoInput", name);
}

QString ShotcutSettings::glaxnimatePath() const
{
    QDir dir(qApp->applicationDirPath());
    return settings.value("glaxnimatePath", dir.absoluteFilePath("glaxnimate")).toString();
}

void ShotcutSettings::setGlaxnimatePath(const QString &path)
{
    settings.setValue("glaxnimatePath", path);
}

bool ShotcutSettings::exportRangeMarkers() const
{
    return settings.value("exportRangeMarkers", true).toBool();
}

void ShotcutSettings::setExportRangeMarkers(bool b)
{
    settings.setValue("exportRangeMarkers", b);
}

bool ShotcutSettings::proxyEnabled() const
{
    return settings.value("proxy/enabled", false).toBool();
}

void ShotcutSettings::setProxyEnabled(bool b)
{
    settings.setValue("proxy/enabled", b);
}

QString ShotcutSettings::proxyFolder() const
{
    QDir dir(appDataLocation());
    const char *subfolder = "proxies";
    if (!dir.cd(subfolder)) {
        if (dir.mkdir(subfolder))
            dir.cd(subfolder);
    }
    return settings.value("proxy/folder", dir.path()).toString();
}

void ShotcutSettings::setProxyFolder(const QString &path)
{
    settings.setValue("proxy/folder", path);
}

bool ShotcutSettings::proxyUseProjectFolder() const
{
    return settings.value("proxy/useProjectFolder", true).toBool();
}

void ShotcutSettings::setProxyUseProjectFolder(bool b)
{
    settings.setValue("proxy/useProjectFolder", b);
}

bool ShotcutSettings::proxyUseHardware() const
{
    return settings.value("proxy/useHardware", false).toBool();
}

void ShotcutSettings::setProxyUseHardware(bool b)
{
    settings.setValue("proxy/useHardware", b);
}

void ShotcutSettings::clearShortcuts(const QString &name)
{
    QString key = "shortcuts/" + name;
    settings.remove(key);
}

void ShotcutSettings::setShortcuts(const QString &name, const QList<QKeySequence> &shortcuts)
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

QList<QKeySequence> ShotcutSettings::shortcuts(const QString &name)
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

double ShotcutSettings::slideshowImageDuration(double defaultSeconds) const
{
    return settings.value("slideshow/clipDuration", defaultSeconds).toDouble();
}

void ShotcutSettings::setSlideshowImageDuration(double seconds)
{
    settings.setValue("slideshow/clipDuration", seconds);
}

double ShotcutSettings::slideshowAudioVideoDuration(double defaultSeconds) const
{
    return settings.value("slideshow/audioVideoDuration", defaultSeconds).toDouble();
}

void ShotcutSettings::setSlideshowAudioVideoDuration(double seconds)
{
    settings.setValue("slideshow/audioVideoDuration", seconds);
}

int ShotcutSettings::slideshowAspectConversion(int defaultAspectConversion) const
{
    return settings.value("slideshow/aspectConversion", defaultAspectConversion).toInt();
}

void ShotcutSettings::setSlideshowAspectConversion(int aspectConversion)
{
    settings.setValue("slideshow/aspectConversion", aspectConversion);
}

int ShotcutSettings::slideshowZoomPercent(int defaultZoomPercent) const
{
    return settings.value("slideshow/zoomPercent", defaultZoomPercent).toInt();
}

void ShotcutSettings::setSlideshowZoomPercent(int zoomPercent)
{
    settings.setValue("slideshow/zoomPercent", zoomPercent);
}

double ShotcutSettings::slideshowTransitionDuration(double defaultTransitionDuration) const
{
    return settings.value("slideshow/transitionDuration", defaultTransitionDuration).toDouble();
}

void ShotcutSettings::setSlideshowTransitionDuration(double transitionDuration)
{
    settings.setValue("slideshow/transitionDuration", transitionDuration);
}

int ShotcutSettings::slideshowTransitionStyle(int defaultTransitionStyle) const
{
    return settings.value("slideshow/transitionStyle", defaultTransitionStyle).toInt();
}

void ShotcutSettings::setSlideshowTransitionStyle(int transitionStyle)
{
    settings.setValue("slideshow/transitionStyle", transitionStyle);
}

int ShotcutSettings::slideshowTransitionSoftness(int defaultTransitionStyle) const
{
    return settings.value("slideshow/transitionSoftness", defaultTransitionStyle).toInt();
}

void ShotcutSettings::setSlideshowTransitionSoftness(int transitionSoftness)
{
    settings.setValue("slideshow/transitionSoftness", transitionSoftness);
}

bool ShotcutSettings::keyframesDragScrub() const
{
    return settings.value("keyframes/dragScrub", false).toBool();
}

void ShotcutSettings::setKeyframesDragScrub(bool b)
{
    settings.setValue("keyframes/dragScrub", b);
    emit keyframesDragScrubChanged();
}

void ShotcutSettings::setSubtitlesShowColumn(const QString &column, bool b)
{
    settings.setValue("subtitles/columns/" + column, b);
}

bool ShotcutSettings::subtitlesShowColumn(const QString &column) const
{
    return settings.value("subtitles/columns/" + column, true).toBool();
}

void ShotcutSettings::setSubtitlesTrackTimeline(bool b)
{
    settings.setValue("subtitles/trackTimeline", b);
}

bool ShotcutSettings::subtitlesTrackTimeline() const
{
    return settings.value("subtitles/trackTimeline", true).toBool();
}

void ShotcutSettings::setSubtitlesShowPrevNext(bool b)
{
    settings.setValue("subtitles/showPrevNext", b);
}

bool ShotcutSettings::subtitlesShowPrevNext() const
{
    return settings.value("subtitles/showPrevNext", true).toBool();
}

QString ShotcutSettings::speechLanguage() const
{
    return settings.value("speech/language", QStringLiteral("a")).toString();
}

void ShotcutSettings::setSpeechLanguage(const QString &code)
{
    settings.setValue("speech/language", code);
}

QString ShotcutSettings::speechVoice() const
{
    return settings.value("speech/voice", QString()).toString();
}

void ShotcutSettings::setSpeechVoice(const QString &voiceId)
{
    settings.setValue("speech/voice", voiceId);
}

double ShotcutSettings::speechSpeed() const
{
    return settings.value("speech/speed", 1.0).toDouble();
}

void ShotcutSettings::setSpeechSpeed(double speed)
{
    settings.setValue("speech/speed", speed);
}

void ShotcutSettings::saveCustomColors()
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

void ShotcutSettings::restoreCustomColors()
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

void ShotcutSettings::setWhisperExe(const QString &path)
{
    settings.setValue("subtitles/whisperExe", path);
}

QString ShotcutSettings::whisperExe()
{
    QDir dir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
    auto exe = "whisper-cli.exe";
#else
    auto exe = "whisper-cli";
#endif
    return settings.value("subtitles/whisperExe", dir.absoluteFilePath(exe)).toString();
}

void ShotcutSettings::setWhisperModel(const QString &path)
{
    settings.setValue("subtitles/whisperModel", path);
}

QString ShotcutSettings::whisperModel()
{
    QDir dataPath = QmlApplication::dataDir();
    dataPath.cd("shotcut/whisper_models");
    return settings.value("subtitles/whisperModel", "").toString();
}

void ShotcutSettings::setNotesZoom(int zoom)
{
    settings.setValue("notes/zoom", zoom);
}

int ShotcutSettings::notesZoom() const
{
    return settings.value("notes/zoom", 0).toInt();
}

void ShotcutSettings::reset()
{
    for (auto &key : settings.allKeys()) {
        settings.remove(key);
    }
}

int ShotcutSettings::undoLimit() const
{
    return settings.value("undoLimit", 50).toInt();
}

bool ShotcutSettings::warnLowMemory() const
{
    return settings.value("warnLowMemory", true).toBool();
}

int ShotcutSettings::backupPeriod() const
{
    return settings.value("backupPeriod", 24 * 60).toInt();
}

void ShotcutSettings::setBackupPeriod(int minutes)
{
    settings.setValue("backupPeriod", minutes);
}

mlt_time_format ShotcutSettings::timeFormat() const
{
    return (mlt_time_format) settings.value("timeFormat", mlt_time_clock).toInt();
}

void ShotcutSettings::setTimeFormat(int format)
{
    settings.setValue("timeFormat", format);
    emit timeFormatChanged();
}

bool ShotcutSettings::askFlatpakWrappers()
{
    return settings.value("flatpakWrappers", true).toBool();
}

void ShotcutSettings::setAskFlatpakWrappers(bool b)
{
    settings.setValue("flatpakWrappers", b);
}

QString ShotcutSettings::dockerPath() const
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

void ShotcutSettings::setDockerPath(const QString &path)
{
    settings.setValue("dockerPath", path);
}

QString ShotcutSettings::chromiumPath() const
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

void ShotcutSettings::setChromiumPath(const QString &path)
{
    settings.setValue("chromiumPath", path);
}

QString ShotcutSettings::screenRecorderPath() const
{
    return settings.value("screenRecorderPath", "obs").toString();
}

void ShotcutSettings::setScreenRecorderPath(const QString &path)
{
    settings.setValue("screenRecorderPath", path);
}
