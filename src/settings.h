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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QByteArray>

class ShotcutSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool timelineShowWaveforms READ timelineShowWaveforms WRITE setTimelineShowWaveforms NOTIFY timelineShowWaveformsChanged)
    Q_PROPERTY(bool timelineShowThumbnails READ timelineShowThumbnails WRITE setTimelineShowThumbnails NOTIFY timelineShowThumbnailsChanged)
    Q_PROPERTY(bool timelineRippleAllTracks READ timelineRippleAllTracks WRITE setTimelineRippleAllTracks NOTIFY timelineRippleAllTracksChanged)
    Q_PROPERTY(QString openPath READ openPath WRITE setOpenPath NOTIFY openPathChanged)
    Q_PROPERTY(QString savePath READ savePath WRITE setSavePath NOTIFY savePathChanged)
    Q_PROPERTY(QString playlistThumbnails READ playlistThumbnails WRITE setPlaylistThumbnails NOTIFY playlistThumbnailsChanged)
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(bool playerAudioChannels READ playerAudioChannels NOTIFY playerAudioChannelsChanged)
    Q_PROPERTY(bool playerGPU READ playerGPU NOTIFY playerGpuChanged)
    Q_PROPERTY(double audioInDuration READ audioInDuration WRITE setAudioInDuration NOTIFY audioInDurationChanged)
    Q_PROPERTY(double audioOutDuration READ audioOutDuration WRITE setAudioOutDuration NOTIFY audioOutDurationChanged)
    Q_PROPERTY(double videoInDuration READ videoInDuration WRITE setVideoInDuration NOTIFY videoInDurationChanged)
    Q_PROPERTY(double videoOutDuration READ videoOutDuration WRITE setVideoOutDuration NOTIFY videoOutDurationChanged)


public:
    static ShotcutSettings& singleton();
    explicit ShotcutSettings() : QObject() {}
    explicit ShotcutSettings(const QString& appDataLocation);

    QString language() const;
    void setLanguage(const QString&);
    double imageDuration() const;
    void setImageDuration(double);
    QString openPath() const;
    void setOpenPath(const QString&);
    QString savePath() const;
    void setSavePath(const QString&);
    QStringList recent() const;
    void setRecent(const QStringList&);
    QString theme() const;
    void setTheme(const QString&);
    bool showTitleBars() const;
    void setShowTitleBars(bool);
    bool showToolBar() const;
    void setShowToolBar(bool);
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray&);
    QByteArray windowGeometryDefault() const;
    void setWindowGeometryDefault(const QByteArray&);
    QByteArray windowState() const;
    void setWindowState(const QByteArray&);
    QByteArray windowStateDefault() const;
    void setWindowStateDefault(const QByteArray&);
    QString viewMode() const;
    void setViewMode(const QString& viewMode);

    QString encodePath() const;
    void setEncodePath(const QString&);
    bool encodeFreeSpaceCheck() const;
    void setEncodeFreeSpaceCheck(bool);
    bool showConvertClipDialog() const;
    void setShowConvertClipDialog(bool);

    bool meltedEnabled() const;
    void setMeltedEnabled(bool);
    QStringList meltedServers() const;
    void setMeltedServers(const QStringList&);

    int playerAudioChannels() const;
    void setPlayerAudioChannels(int);
    QString playerDeinterlacer() const;
    void setPlayerDeinterlacer(const QString&);
    QString playerExternal() const;
    void setPlayerExternal(const QString&);
    QString playerGamma() const;
    void setPlayerGamma(const QString&);
    bool playerGPU() const;
    void setPlayerGPU(bool);
    QString playerInterpolation() const;
    void setPlayerInterpolation(const QString&);
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
    bool playerScrubAudio() const;
    void setPlayerScrubAudio(bool);
    int playerVolume() const;
    void setPlayerVolume(int);
    float playerZoom() const;
    void setPlayerZoom(float);

    QString playlistThumbnails() const;
    void setPlaylistThumbnails(const QString&);

    bool timelineShowWaveforms() const;
    void setTimelineShowWaveforms(bool);
    bool timelineShowThumbnails() const;
    void setTimelineShowThumbnails(bool);
    bool timelineRippleAllTracks() const;
    void setTimelineRippleAllTracks(bool);

    QString filterFavorite(const QString& filterName);
    void setFilterFavorite(const QString& filterName, const QString& value);

    double audioInDuration() const;
    void setAudioInDuration(double);

    double audioOutDuration() const;
    void setAudioOutDuration(double);

    double videoInDuration() const;
    void setVideoInDuration(double);

    double videoOutDuration() const;
    void setVideoOutDuration(double);

    bool loudnessScopeShowMeter(const QString& meter) const;
    void setLoudnessScopeShowMeter(const QString& meter, bool b);

    int drawMethod() const;
    void setDrawMethod(int);

    bool noUpgrade() const;
    void setNoUpgrade(bool value);

    void sync();
    QString appDataLocation() const;
    void setAppDataForSession(const QString& location);
    void setAppDataLocally(const QString& location);

signals:
    void openPathChanged();
    void savePathChanged();
    void timelineShowWaveformsChanged();
    void timelineShowThumbnailsChanged();
    void timelineRippleAllTracksChanged();
    void playerAudioChannelsChanged(int);
    void playerGpuChanged();
    void audioInDurationChanged();
    void audioOutDurationChanged();
    void videoInDurationChanged();
    void videoOutDurationChanged();
    void playlistThumbnailsChanged();
    void viewModeChanged();

private:
    QSettings settings;
    QString m_appDataLocation;
};

#define Settings ShotcutSettings::singleton()

#endif // SETTINGS_H
