/*
 * Copyright (c) 2026 Meltytech, LLC
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

#ifndef HDRPREVIEWWINDOW_H
#define HDRPREVIEWWINDOW_H

#include "videowidget.h"
#include <QKeyEvent>
#include <QPointer>
#include <QQuickView>
#include <QRect>
#include <QResizeEvent>
#include <QString>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

class HdrPreviewWindow : public QQuickView
{
    Q_OBJECT
    Q_PROPERTY(float hdrGain READ hdrGain NOTIFY hdrGainChanged)
    Q_PROPERTY(int hdrTransferMode READ hdrTransferMode NOTIFY hdrTransferModeChanged)
    Q_PROPERTY(int displayPeakNits READ displayPeakNits WRITE setDisplayPeakNits NOTIFY
                   displayPeakNitsChanged)
    Q_PROPERTY(int contentPeakNits READ contentPeakNits WRITE setContentPeakNits NOTIFY
                   contentPeakNitsChanged)
    Q_PROPERTY(bool toneMapping READ toneMapping WRITE setToneMapping NOTIFY toneMappingChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool fullScreen READ isFullScreen NOTIFY fullScreenChanged)
    Q_PROPERTY(int videoPosition READ videoPosition NOTIFY videoPositionChanged)
    Q_PROPERTY(int videoDuration READ videoDuration NOTIFY videoDurationChanged)
    Q_PROPERTY(QString positionText READ positionText NOTIFY videoPositionChanged)
    Q_PROPERTY(QString durationText READ durationText NOTIFY videoDurationChanged)

public:
    explicit HdrPreviewWindow(QWindow *parent = nullptr);
    ~HdrPreviewWindow();

    Q_INVOKABLE void setVideoSink(QVideoSink *sink);
    float hdrGain() const { return m_hdrGain; }
    int hdrTransferMode() const { return static_cast<int>(m_hdrTransfer); }
    bool isHdrAvailable() const;
    int displayPeakNits() const { return m_displayPeakNits; }
    void setDisplayPeakNits(int nits);
    int contentPeakNits() const { return m_contentPeakNits; }
    void setContentPeakNits(int nits);
    bool toneMapping() const { return m_toneMapping; }
    void setToneMapping(bool enabled);
    bool isPlaying() const { return m_isPlaying; }
    /// Restore a previously saved window geometry without triggering the
    /// DAR-snap in resizeEvent (which would grow the window on each launch).
    void restoreGeometry(const QRect &r);
    void setNormalGeometry(const QRect &r);
    bool isFullScreen() const { return windowStates() & Qt::WindowFullScreen; }
    int videoPosition() const { return m_videoPosition; }
    int videoDuration() const { return m_videoDuration; }
    QString positionText() const;
    QString durationText() const;

    Q_INVOKABLE void triggerPlayPause();
    Q_INVOKABLE void triggerRewind();
    Q_INVOKABLE void triggerFastForward();
    Q_INVOKABLE void toggleFullScreen();
    Q_INVOKABLE void seekToFrame(int frame);

public slots:
    void pushFrame(const QVideoFrame &frame);
    void setHdrTransfer(HdrTransfer transfer);
    void setPlaying(bool playing);

signals:
    void hdrGainChanged();
    void hdrTransferModeChanged();
    void hdrModeRestartRequested();
    void displayPeakNitsChanged();
    void contentPeakNitsChanged();
    void toneMappingChanged();
    void playingChanged();
    void fullScreenChanged();
    void videoPositionChanged();
    void videoDurationChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void checkEdrHeadroom();
    void onScreenChanged(QScreen *screen);
    void processPendingScreenChange();

private:
    bool isHdrMode() const;
    void updateHdrGain();
    void invalidateVideoNode();

    QPointer<QVideoSink> m_videoSink;
    QPointer<QScreen> m_lastScreen;
    QPointer<QScreen> m_pendingScreen;
    QTimer m_edrTimer;
    QTimer m_screenChangeTimer;
    bool m_loggedSwapChain{false};
    bool m_loggedGainSkip{false};
    bool m_lastKnownHdrMode{false};
    bool m_pendingWasHdrMode{false};
    bool m_warnedAboutScreenChange{false};
    bool m_skipNextFrame{false};
    HdrTransfer m_hdrTransfer{HdrTransfer::SDR};
    bool m_isPlaying{false};
    float m_lastLoggedHeadroom{0.0f};
    int m_edrCheckCount{0};
    float m_hdrGain{1.0f};
    int m_displayPeakNits{0};
    int m_contentPeakNits{0};
    bool m_toneMapping{true};
    bool m_skipDarSnap{false};
    QRect m_normalGeometry;
    int m_videoPosition{0};
    int m_videoDuration{0};
};

#endif // HDRPREVIEWWINDOW_H
