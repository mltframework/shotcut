/*
 * Copyright (c) 2013 Meltytech, LLC
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

ShotcutSettings &ShotcutSettings::singleton()
{
    static ShotcutSettings* instance = new ShotcutSettings;
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

QString ShotcutSettings::openPath() const
{
    return settings.value("openPath", QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)).toString();
}

void ShotcutSettings::setOpenPath(const QString& s)
{
    settings.setValue("openPath", s);
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

QByteArray ShotcutSettings::windowGeometry() const
{
    return settings.value("geometry").toByteArray();
}

void ShotcutSettings::setWindowGeometry(const QByteArray& a)
{
    settings.setValue("geometry", a);
}

QByteArray ShotcutSettings::windowState() const
{
    return settings.value("windowState").toByteArray();
}

void ShotcutSettings::setWindowState(const QByteArray& a)
{
    settings.setValue("windowState", a);
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

void ShotcutSettings::setPlayerGPU(bool b)
{
    settings.setValue("player/gpu", b);
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

bool ShotcutSettings::playerOpenGL() const
{
    return settings.value("player/opengl", true).toBool();
}

void ShotcutSettings::setPlayerOpenGL(bool b)
{
    settings.setValue("player/opengl", b);
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

int ShotcutSettings::playerVolume() const
{
    return settings.value("player/volume", 88).toInt();
}

void ShotcutSettings::setPlayerVolume(int i)
{
    settings.setValue("player/volume", i);
}

bool ShotcutSettings::playerVolumeVisible() const
{
    return settings.value("player/volume-visible", false).toBool();
}

void ShotcutSettings::setPlayerVolumeVisible(bool b)
{
    settings.setValue("player/volume-visible", b);
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
}

void ShotcutSettings::sync()
{
    settings.sync();
}
