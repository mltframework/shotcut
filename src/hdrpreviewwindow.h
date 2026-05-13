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

#include <QPointer>
#include <QQuickView>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

class HdrPreviewWindow : public QQuickView
{
    Q_OBJECT
    Q_PROPERTY(float hdrGain READ hdrGain NOTIFY hdrGainChanged)

public:
    explicit HdrPreviewWindow(QWindow *parent = nullptr);
    ~HdrPreviewWindow();

    Q_INVOKABLE void setVideoSink(QVideoSink *sink);
    float hdrGain() const { return m_hdrGain; }

public slots:
    void pushFrame(const QVideoFrame &frame);
    void setHlg(bool isHlg);

signals:
    void hdrGainChanged();

private slots:
    void checkEdrHeadroom();

private:
    void updateHdrGain();

    QPointer<QVideoSink> m_videoSink;
    QTimer m_edrTimer;
    bool m_loggedSwapChain{false};
    bool m_isHlg{false};
    float m_lastLoggedHeadroom{0.0f};
    int m_edrCheckCount{0};
    float m_hdrGain{1.0f};
};

#endif // HDRPREVIEWWINDOW_H
