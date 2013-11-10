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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QStringList>
#include <QByteArray>

class ShotcutSettings
{
public:
    static ShotcutSettings& singleton();

    QString language() const;
    void setLanguage(const QString&);
    QString openPath() const;
    void setOpenPath(const QString&);
    QStringList recent() const;
    void setRecent(const QStringList&);
    QString theme() const;
    void setTheme(const QString&);
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray&);
    QByteArray windowState() const;
    void setWindowState(const QByteArray&);

    QString encodePath() const;
    void setEncodePath(const QString&);

    QStringList meltedServers() const;
    void setMeltedServers(const QStringList&);

    QString playerDeinterlacer() const;
    void setPlayerDeinterlacer(const QString&);
    QString playerExternal() const;
    void setPlayerExternal(const QString&);
    bool playerGPU() const;
    void setPlayerGPU(bool);
    QString playerInterpolation() const;
    void setPlayerInterpolation(const QString&);
    bool playerOpenGL() const;
    void setPlayerOpenGL(bool);
    bool playerJACK() const;
    void setPlayerJACK(bool);
    int playerKeyerMode() const;
    void setPlayerKeyerMode(int);
    bool playerMuted() const;
    void setPlayerMuted(bool);
    QString playerProfile() const;
    void setPlayerProfile(const QString&);
    bool playerProgressive() const;
    void setPlayerProgressive(bool);
    bool playerRealtime() const;
    void setPlayerRealtime(bool);
    int playerVolume() const;
    void setPlayerVolume(int);
    bool playerVolumeVisible() const;
    void setPlayerVolumeVisible(bool);
    float playerZoom() const;
    void setPlayerZoom(float);

    QString playlistThumbnails() const;
    void setPlaylistThumbnails(const QString&);

    void sync();

private:
    QSettings settings;
};

#define Settings ShotcutSettings::singleton()

#endif // SETTINGS_H
