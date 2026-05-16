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
    bool isPlaying() const { return m_isPlaying; }
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

private:
    void updateHdrGain();

    QPointer<QVideoSink> m_videoSink;
    QTimer m_edrTimer;
    bool m_loggedSwapChain{false};
    bool m_loggedGainSkip{false};
    HdrTransfer m_hdrTransfer{HdrTransfer::SDR};
    bool m_isPlaying{false};
    float m_lastLoggedHeadroom{0.0f};
    int m_edrCheckCount{0};
    float m_hdrGain{1.0f};
    QRect m_normalGeometry;
    int m_videoPosition{0};
    int m_videoDuration{0};
};

#endif // HDRPREVIEWWINDOW_H
