/*
 * Copyright (c) 2013-2017 Meltytech, LLC
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

#include "settings.h"
#include <QLocale>
#include <QStandardPaths>
#include <QFile>
#include <Logger.h>

static const QString APP_DATA_DIR_KEY("appdatadir");
static const QString SHOTCUT_INI_FILENAME("/shotcut.ini");
static QScopedPointer<ShotcutSettings> instance;

ShotcutSettings &ShotcutSettings::singleton()
{
    if (!instance) {
        instance.reset(new ShotcutSettings);
        if (instance->settings.value(APP_DATA_DIR_KEY).isValid()
            && QFile::exists(instance->settings.value(APP_DATA_DIR_KEY).toString() + SHOTCUT_INI_FILENAME) )
            instance.reset(new ShotcutSettings(instance->settings.value(APP_DATA_DIR_KEY).toString()));
        LOG_DEBUG() << "language" << instance->language();
        LOG_DEBUG() << "deinterlacer" << instance->playerDeinterlacer();
        LOG_DEBUG() << "external monitor" << instance->playerExternal();
        LOG_DEBUG() << "GPU processing" << instance->playerGPU();
        LOG_DEBUG() << "interpolation" << instance->playerInterpolation();
        LOG_DEBUG() << "video mode" << instance->playerProfile();
        LOG_DEBUG() << "realtime" << instance->playerRealtime();
    }
    return *instance;
}

ShotcutSettings::ShotcutSettings(const QString& appDataLocation)
    : QObject()
    , settings(appDataLocation + SHOTCUT_INI_FILENAME, QSettings::IniFormat)
    , m_appDataLocation(appDataLocation)
{
}

QString ShotcutSettings::language() const
{
    return settings.value("language", QLocale().name()).toString();
}

void ShotcutSettings::setLanguage(const QString& s)
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
    return settings.value("openPath", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString();
}

void ShotcutSettings::setOpenPath(const QString& s)
{
    settings.setValue("openPath", s);
    emit savePathChanged();
}

QString ShotcutSettings::savePath() const
{
    return settings.value("savePath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString();
}

void ShotcutSettings::setSavePath(const QString& s)
{
    settings.setValue("savePath", s);
    emit savePathChanged();
}

QStringList ShotcutSettings::recent() const
{
    return settings.value("recent").toStringList();
}

void ShotcutSettings::setRecent(const QStringList& ls)
{
    settings.setValue("recent", ls);
}

QString ShotcutSettings::theme() const
{
    return settings.value("theme", "dark").toString();
}

void ShotcutSettings::setTheme(const QString& s)
{
    settings.setValue("theme", s);
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

QByteArray ShotcutSettings::windowGeometry() const
{
    return settings.value("geometry").toByteArray();
}

void ShotcutSettings::setWindowGeometry(const QByteArray& a)
{
    settings.setValue("geometry", a);
}

QByteArray ShotcutSettings::windowGeometryDefault() const
{
    return settings.value("geometryDefault").toByteArray();
}

void ShotcutSettings::setWindowGeometryDefault(const QByteArray& a)
{
    settings.setValue("geometryDefault", a);
}

QByteArray ShotcutSettings::windowState() const
{
    return settings.value("windowState").toByteArray();
}

void ShotcutSettings::setWindowState(const QByteArray& a)
{
    settings.setValue("windowState", a);
}

QByteArray ShotcutSettings::windowStateDefault() const
{
    return settings.value("windowStateDefault").toByteArray();
}

void ShotcutSettings::setWindowStateDefault(const QByteArray& a)
{
    settings.setValue("windowStateDefault", a);
}

QString ShotcutSettings::viewMode() const
{
    return settings.value("playlist/viewMode").toString();
}

void ShotcutSettings::setViewMode(const QString& viewMode)
{
    settings.setValue("playlist/viewMode", viewMode);
    emit viewModeChanged();
}

QString ShotcutSettings::encodePath() const
{
    return settings.value("encode/path", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString();
}

void ShotcutSettings::setEncodePath(const QString& s)
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

bool ShotcutSettings::showConvertClipDialog() const
{
    return settings.value("showConvertClipDialog", true).toBool();
}

void ShotcutSettings::setShowConvertClipDialog(bool b)
{
    settings.setValue("showConvertClipDialog", b);
}

bool ShotcutSettings::meltedEnabled() const
{
    return settings.value("melted/enabled", false).toBool();
}

void ShotcutSettings::setMeltedEnabled(bool b)
{
    settings.setValue("melted/enabled", b);
}

QStringList ShotcutSettings::meltedServers() const
{
    return settings.value("melted/servers").toStringList();
}

void ShotcutSettings::setMeltedServers(const QStringList& ls)
{
    settings.setValue("melted/servers", ls);
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
    return settings.value("player/deinterlacer", "onefield").toString();
}

void ShotcutSettings::setPlayerDeinterlacer(const QString& s)
{
    settings.setValue("player/deinterlacer", s);
}

QString ShotcutSettings::playerExternal() const
{
    return settings.value("player/external", "").toString();
}

void ShotcutSettings::setPlayerExternal(const QString& s)
{
    settings.setValue("player/external", s);
}

QString ShotcutSettings::playerGamma() const
{
    return settings.value("player/gamma", "bt709").toString();
}

void ShotcutSettings::setPlayerGamma(const QString& s)
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
    return settings.value("player/interpolation", "nearest").toString();
}

void ShotcutSettings::setPlayerInterpolation(const QString& s)
{
    settings.setValue("player/interpolation", s);
}

bool ShotcutSettings::playerGPU() const
{
    return settings.value("player/gpu", false).toBool();
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

void ShotcutSettings::setPlayerProfile(const QString& s)
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

QString ShotcutSettings::playlistThumbnails() const
{
    return settings.value("playlist/thumbnails", "small").toString();
}

void ShotcutSettings::setPlaylistThumbnails(const QString& s)
{
    settings.setValue("playlist/thumbnails", s);
    emit playlistThumbnailsChanged();
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

bool ShotcutSettings::timelineRippleAllTracks() const
{
    return settings.value("timeline/rippleAllTracks", false).toBool();
}

void ShotcutSettings::setTimelineRippleAllTracks(bool b)
{
    settings.setValue("timeline/rippleAllTracks", b);
    emit timelineRippleAllTracksChanged();
}

QString ShotcutSettings::filterFavorite(const QString& filterName)
{
    return settings.value("filter/favorite/" + filterName, "").toString();
}
void ShotcutSettings::setFilterFavorite(const QString& filterName, const QString& value)
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

bool ShotcutSettings::loudnessScopeShowMeter(const QString& meter) const
{
    return settings.value("scope/loudness/" + meter, true).toBool();
}

void ShotcutSettings::setLoudnessScopeShowMeter(const QString& meter, bool b)
{
    settings.setValue("scope/loudness/" + meter, b);
}

int ShotcutSettings::drawMethod() const
{
    return settings.value("opengl", Qt::AA_UseOpenGLES).toInt();
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

void ShotcutSettings::setAppDataForSession(const QString& location)
{
    // This is intended to be called when using a command line option
    // to set the AppData location.
    instance.reset(new ShotcutSettings(location));
}

void ShotcutSettings::setAppDataLocally(const QString& location)
{
    // This is intended to be called when using a GUI action to set the
    // the new AppData location.

    // Copy the existing settings if they exist.
    if (!QFile::exists(location + SHOTCUT_INI_FILENAME)) {
        QSettings newSettings(location + SHOTCUT_INI_FILENAME, QSettings::IniFormat);
        foreach (const QString& key, settings.allKeys())
            newSettings.setValue(key, settings.value(key));
        newSettings.sync();
    }

    // Set the new location.
    QSettings localSettings;
    localSettings.setValue(APP_DATA_DIR_KEY, location);
    localSettings.sync();
}
