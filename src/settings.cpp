/*
 * Copyright (c) 2013-2022 Meltytech, LLC
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
#include <QColor>
#include <QLocale>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QAudioDeviceInfo>
#include <Logger.h>

static const QString APP_DATA_DIR_KEY("appdatadir");
static const QString SHOTCUT_INI_FILENAME("/shotcut.ini");
static QScopedPointer<ShotcutSettings> instance;
static QString appDataForSession;
static const int kMaximumTrackHeight = 125;

ShotcutSettings &ShotcutSettings::singleton()
{
    if (!instance) {
        if (appDataForSession.isEmpty()) {
            instance.reset(new ShotcutSettings);
            if (instance->settings.value(APP_DATA_DIR_KEY).isValid()
                    && QFile::exists(instance->settings.value(APP_DATA_DIR_KEY).toString() + SHOTCUT_INI_FILENAME) )
                instance.reset(new ShotcutSettings(instance->settings.value(APP_DATA_DIR_KEY).toString()));
        } else {
            instance.reset(new ShotcutSettings(appDataForSession));
        }
    }
    return *instance;
}

ShotcutSettings::ShotcutSettings(const QString &appDataLocation)
    : QObject()
    , settings(appDataLocation + SHOTCUT_INI_FILENAME, QSettings::IniFormat)
    , m_appDataLocation(appDataLocation)
{
}

void ShotcutSettings::log()
{
    LOG_DEBUG() << "language" << language();
    LOG_DEBUG() << "deinterlacer" << playerDeinterlacer();
    LOG_DEBUG() << "external monitor" << playerExternal();
    LOG_DEBUG() << "GPU processing" << playerGPU();
    LOG_DEBUG() << "interpolation" << playerInterpolation();
    LOG_DEBUG() << "video mode" << playerProfile();
    LOG_DEBUG() << "realtime" << playerRealtime();
    LOG_DEBUG() << "audio channels" << playerAudioChannels();
#ifdef Q_OS_WIN
    LOG_DEBUG() << "display method" << drawMethod();
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
    return settings.value("openPath",
                          QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString();
}

void ShotcutSettings::setOpenPath(const QString &s)
{
    settings.setValue("openPath", s);
    emit savePathChanged();
}

QString ShotcutSettings::savePath() const
{
    return settings.value("savePath",
                          QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString();
}

void ShotcutSettings::setSavePath(const QString &s)
{
    settings.setValue("savePath", s);
    emit savePathChanged();
}

QStringList ShotcutSettings::recent() const
{
    return settings.value("recent").toStringList();
}

void ShotcutSettings::setRecent(const QStringList &ls)
{
    if (ls.isEmpty())
        settings.remove("recent");
    else if (!clearRecent())
        settings.setValue("recent", ls);
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
    return settings.value("geometry").toByteArray();
}

void ShotcutSettings::setWindowGeometry(const QByteArray &a)
{
    settings.setValue("geometry", a);
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
    return settings.value("windowState").toByteArray();
}

void ShotcutSettings::setWindowState(const QByteArray &a)
{
    settings.setValue("windowState", a);
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
    return settings.value("encode/path",
                          QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString();
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

QString ShotcutSettings::playerGamma() const
{
    return settings.value("player/gamma", "bt709").toString();
}

void ShotcutSettings::setPlayerGamma(const QString &s)
{
    settings.setValue("player/gamma", s);
}

void ShotcutSettings::setPlayerGPU(bool b)
{
    settings.setValue("player/gpu", b);
    emit playerGpuChanged();
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
    return settings.value("player/gpu", false).toBool();
}

bool ShotcutSettings::playerWarnGPU() const
{
    return settings.value("player/warnGPU", false).toBool();
}

void ShotcutSettings::setPlayerJACK(bool b)
{
    settings.setValue("player/jack", b);
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

bool ShotcutSettings::timelineCenterPlayhead() const
{
    return settings.value("timeline/centerPlayhead", false).toBool();
}

void ShotcutSettings::setTimelineCenterPlayhead(bool b)
{
    settings.setValue("timeline/centerPlayhead", b);
    emit timelineCenterPlayheadChanged();
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
    return (Qt::SortOrder)settings.value("markers/sortOrder", Qt::AscendingOrder).toInt();
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

bool ShotcutSettings::setLayout(const QString &name, const QByteArray &geometry,
                                const QByteArray &state)
{
    bool isNew = false;
    QStringList layouts = Settings.layouts();
    if (layouts.indexOf(name) == -1) {
        isNew = true;
        layouts.append(name);
        settings.setValue("layout/layouts", layouts);
    }
    settings.setValue(QString("layout/%1_%2").arg(name).arg("geometry"), geometry);
    settings.setValue(QString("layout/%1_%2").arg(name).arg("state"), state);
    return isNew;
}

QByteArray ShotcutSettings::layoutGeometry(const QString &name)
{
    QString key = QString("layout/%1_geometry").arg(name);
    return settings.value(key).toByteArray();
}

QByteArray ShotcutSettings::layoutState(const QString &name)
{
    QString key = QString("layout/%1_state").arg(name);
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
        settings.remove(QString("layout/%1_%2").arg(name).arg("geometry"));
        settings.remove(QString("layout/%1_%2").arg(name).arg("state"));
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
    return settings.value("projectsFolder",
                          QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString();
}

void ShotcutSettings::setProjectsFolder(const QString &path)
{
    settings.setValue("projectsFolder", path);
}

QString ShotcutSettings::audioInput() const
{
    QString defaultValue  = "default";
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    for (const auto &deviceInfo : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        defaultValue = deviceInfo.deviceName();
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
    return settings.value("glaxnimatePath", "glaxnimate").toString();
}

void ShotcutSettings::setGlaxnimatePath(const QString &path)
{
    settings.setValue("glaxnimatePath", path);
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

int ShotcutSettings::undoLimit() const
{
    return settings.value("undoLimit", 1000).toInt();
}
