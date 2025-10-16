/*
 * Copyright (c) 2025 Meltytech, LLC
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

#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <memory>
#include <QObject>
#include <QRect>
#include <QScreen>
#include <QVariant>

class ToolbarWidget;
class RectangleSelector;
class WindowPicker;

class ScreenCapture : public QObject
{
    Q_OBJECT

public:
    enum CaptureMode { Fullscreen, Rectangle, Window, Interactive };

    explicit ScreenCapture(const QString &outputFile, CaptureMode mode, QObject *parent = nullptr);
    ~ScreenCapture();

    void startRecording();
    void startSnapshot();
    static bool isWayland();

signals:
    void finished(bool success);
    void beginRecording(const QRect &captureRect);
    void onSelectionCanceled();

private slots:
    void onCaptureModeSelected(CaptureMode mode);
    void onRectangleSelected(const QRect &rect);
    void onWindowSelected(const QRect &rect);
    void onImageRectangleSelected(const QRect &rect);
    void onImageWindowSelected(const QRect &rect);
    // xdg-desktop-portal response handler
    void onPortalResponse(uint response, const QVariantMap &results);

private:
    void startFullscreenRecording();
    void startRectangleRecording();
    void startWindowRecording();
    void startFullscreenSnapshot();
    void startRectangleSnapshot();
    void startWindowSnapshot();
    void captureAndSaveImage(const QRect &rect);
    void doCaptureAndSaveImage(const QRect &rect);
    QPixmap captureScreen(const QRect &rect);
    QRect adjustRectForVideo(const QRect &rect);
    QRect applyDevicePixelRatio(const QRect &rect);
    QRect invertDevicePixelRatio(const QRect &rect);
    bool captureImagePortal(const QRect &rect, const QString &outputPath);

    // Portal call state
    bool m_portalSuccess = false;
    QString m_portalUri;
    QEventLoop *m_portalEventLoop = nullptr;

    QString m_outputFile;
    CaptureMode m_mode;
    bool m_isImageMode;

    std::unique_ptr<ToolbarWidget> m_toolbar;
    std::unique_ptr<RectangleSelector> m_rectangleSelector;
    std::unique_ptr<WindowPicker> m_windowPicker;
};

#endif // SCREENCAPTURE_H
