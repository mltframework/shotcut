/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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
#include <QDebug>

ShotcutSettings &ShotcutSettings::singleton()
{
    static ShotcutSettings* instance = 0;
    if (!instance) {
        instance = new ShotcutSettings;
        qDebug() << "language" << instance->language();
        qDebug() << "deinterlacer" << instance->playerDeinterlacer();
        qDebug() << "external monitor" << instance->playerExternal();
        qDebug() << "GPU processing" << instance->playerGPU();
        qDebug() << "interpolation" << instance->playerInterpolation();
        qDebug() << "video mode" << instance->playerProfile();
        qDebug() << "realtime" << instance->playerRealtime();
    }
    return *instance;
}

QString ShotcutSettings::language() const
{
    return settings.value("language", QLocale::system().name()).toString();
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

QString ShotcutSettings::encodePath() const
{
    return settings.value("encode/path", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString();
}

void ShotcutSettings::setEncodePath(const QString& s)
{
    settings.setValue("encode/path", s);
}

QStringList ShotcutSettings::meltedServers() const
{
    return settings.value("melted/servers").toStringList();
}

void ShotcutSettings::setMeltedServers(const QStringList& ls)
{
    settings.setValue("melted/servers", ls);
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
    return settings.value("player/gamma", "iec61966_2_1").toString();
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

int ShotcutSettings::drawMethod() const
{
    return settings.value("opengl", 0).toInt();
}

void ShotcutSettings::setDrawMethod(int i)
{
    settings.setValue("opengl", i);
}

void ShotcutSettings::sync()
{
    settings.sync();
}
