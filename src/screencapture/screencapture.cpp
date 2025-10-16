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

#include "screencapture.h"
#include "rectangleselector.h"
#include "toolbarwidget.h"
#include "windowpicker.h"
#include <QApplication>
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QUrl>
#include <QVariant>

ScreenCapture::ScreenCapture(const QString &outputFile, CaptureMode mode, QObject *parent)
    : QObject(parent)
    , m_outputFile(outputFile)
    , m_mode(mode)
    , m_isImageMode(false)
{}

ScreenCapture::~ScreenCapture() {}

void ScreenCapture::startRecording()
{
    switch (m_mode) {
    case Fullscreen:
        startFullscreenRecording();
        break;
    case Rectangle:
        startRectangleRecording();
        break;
    case Window:
        startWindowRecording();
        break;
    case Interactive:
        m_toolbar = std::make_unique<ToolbarWidget>();
        // Toolbar emits int, convert to CaptureMode via lambda for Qt6 typed connect compatibility
        connect(m_toolbar.get(), &ToolbarWidget::captureModeSelected, this, [this](int mode) {
            this->onCaptureModeSelected(static_cast<CaptureMode>(mode));
        });
        m_toolbar->show();
        break;
    }
}

void ScreenCapture::startSnapshot()
{
    m_isImageMode = true;

    switch (m_mode) {
    case Fullscreen:
        startFullscreenSnapshot();
        break;
    case Rectangle:
        startRectangleSnapshot();
        break;
    case Window:
        startWindowSnapshot();
        break;
    case Interactive:
        // Show toolbar for user to choose mode
        m_toolbar = std::make_unique<ToolbarWidget>();
        connect(m_toolbar.get(), &ToolbarWidget::captureModeSelected, this, [this](int mode) {
            this->onCaptureModeSelected(static_cast<CaptureMode>(mode));
        });
        m_toolbar->show();
        break;
    }
}

void ScreenCapture::onCaptureModeSelected(CaptureMode mode)
{
    // Close the toolbar safely after signal processing completes
    if (m_toolbar) {
        m_toolbar->close();
        m_toolbar->deleteLater();
        m_toolbar.release(); // Release ownership so unique_ptr doesn't delete it
    }

    m_mode = mode;

    // Continue with the selected mode, respecting image vs video
    if (m_isImageMode) {
        startSnapshot();
    } else {
        startRecording();
    }
}

void ScreenCapture::onRectangleSelected(const QRect &rect)
{
    // Close the rectangle selector safely after signal processing completes
    if (m_rectangleSelector) {
        m_rectangleSelector->close();
        m_rectangleSelector->deleteLater();
        m_rectangleSelector.release();
    }

    // Apply device pixel ratio to convert logical to physical pixels
    auto newRect = applyDevicePixelRatio(rect);
    auto screen = QGuiApplication::primaryScreen();
    if (screen)
        newRect &= applyDevicePixelRatio(screen->geometry());

    emit beginRecording(adjustRectForVideo(newRect));
}

void ScreenCapture::onWindowSelected(const QRect &rect)
{
    // Close the window picker safely after signal processing completes
    if (m_windowPicker) {
        m_windowPicker->close();
        m_windowPicker->deleteLater();
        m_windowPicker.release();
    }

    // Constrain the rect in case it has padding for shadows
    auto newRect = applyDevicePixelRatio(rect);
    newRect.setX(rect.x());
    newRect.setY(rect.y());
    auto screen = QGuiApplication::primaryScreen();
    if (screen)
        newRect &= applyDevicePixelRatio(screen->geometry());

    emit beginRecording(adjustRectForVideo(newRect));
}

void ScreenCapture::startFullscreenRecording()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qCritical() << "Error: No screen found";
        return;
    }

    auto physicalRect = applyDevicePixelRatio(screen->geometry());

    emit beginRecording(adjustRectForVideo(physicalRect));
}

void ScreenCapture::startRectangleRecording()
{
    m_rectangleSelector = std::make_unique<RectangleSelector>();
    connect(m_rectangleSelector.get(),
            &RectangleSelector::rectangleSelected,
            this,
            &ScreenCapture::onRectangleSelected);
    connect(m_rectangleSelector.get(),
            &RectangleSelector::canceled,
            this,
            &ScreenCapture::onSelectionCanceled);
    m_rectangleSelector->show();
}

void ScreenCapture::startWindowRecording()
{
    if (isWayland()) {
        qCritical() << "Window capture is not supported on Wayland";
        return;
    }
    m_windowPicker = std::make_unique<WindowPicker>();
    connect(m_windowPicker.get(),
            &WindowPicker::windowSelected,
            this,
            &ScreenCapture::onWindowSelected);
    m_windowPicker->show();
}

QPixmap ScreenCapture::captureScreen(const QRect &rect)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return QPixmap();
    }

    return screen->grabWindow(0, rect.x(), rect.y(), rect.width(), rect.height());
}

void ScreenCapture::startFullscreenSnapshot()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qCritical() << "Error: No screen found";
        return;
    }

    // Small delay to let any UI elements disappear
    QTimer::singleShot(100, this, [this, screen]() { captureAndSaveImage(screen->geometry()); });
}

void ScreenCapture::startRectangleSnapshot()
{
    m_rectangleSelector = std::make_unique<RectangleSelector>();
    connect(m_rectangleSelector.get(),
            &RectangleSelector::rectangleSelected,
            this,
            &ScreenCapture::onImageRectangleSelected);
    connect(m_rectangleSelector.get(),
            &RectangleSelector::canceled,
            this,
            &ScreenCapture::onSelectionCanceled);
    m_rectangleSelector->show();
}

void ScreenCapture::startWindowSnapshot()
{
    if (isWayland()) {
        qCritical() << "Window image capture is not supported on Wayland";
        return;
    }
    m_windowPicker = std::make_unique<WindowPicker>();
    connect(m_windowPicker.get(),
            &WindowPicker::windowSelected,
            this,
            &ScreenCapture::onImageWindowSelected);
    m_windowPicker->show();
}

void ScreenCapture::onImageRectangleSelected(const QRect &rect)
{
    // Close the rectangle selector safely after signal processing completes
    if (m_rectangleSelector) {
        m_rectangleSelector->close();
        m_rectangleSelector->deleteLater();
        m_rectangleSelector.release();
    }
    // Translate from selector widget coords (primary screen) to global coords
    QRect globalRect = rect;
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        globalRect.translate(screen->geometry().topLeft());
    }
    captureAndSaveImage(globalRect);
}

void ScreenCapture::onImageWindowSelected(const QRect &rect)
{
    // Close the window picker safely after signal processing completes
    if (m_windowPicker) {
        m_windowPicker->close();
        m_windowPicker->deleteLater();
        m_windowPicker.release();
    }

    captureAndSaveImage(invertDevicePixelRatio(rect));
}

void ScreenCapture::captureAndSaveImage(const QRect &rect)
{
    // Ensure our UI elements (toolbar/selector) are gone before capture.
    // Close any open tool UIs immediately.
    if (m_toolbar) {
        m_toolbar->close();
        m_toolbar->deleteLater();
        m_toolbar.release();
    }
    if (m_rectangleSelector) {
        m_rectangleSelector->close();
        m_rectangleSelector->deleteLater();
        m_rectangleSelector.release();
    }
    if (m_windowPicker) {
        m_windowPicker->close();
        m_windowPicker->deleteLater();
        m_windowPicker.release();
    }

    // Defer a bit so the window manager/compositor has time to unmap our UI
    // before the portal snapshot is taken.
    if (isWayland()) {
        QTimer::singleShot(120, this, [this, rect]() { doCaptureAndSaveImage(rect); });
    } else {
        doCaptureAndSaveImage(rect);
    }
}

void ScreenCapture::doCaptureAndSaveImage(const QRect &rect)
{
    qDebug() << "Capturing image:" << rect;
    qDebug() << "Output file:" << m_outputFile;

    if (isWayland()) {
        // Try xdg-desktop-portal first (permission prompt)
        if (captureImagePortal(rect, m_outputFile)) {
            qDebug() << "Image saved successfully to:" << m_outputFile;
            emit finished(true);
            return;
        }
        qWarning() << "Portal screenshot failed, will try Qt grabWindow next.";
    }

    QPixmap pixmap = captureScreen(rect);
    if (pixmap.isNull()) {
        qCritical() << "Failed to capture screen";
        emit finished(false);
        return;
    }

    if (pixmap.save(m_outputFile)) {
        qDebug() << "Image saved successfully to:" << m_outputFile;
        emit finished(true);
    } else {
        qCritical() << "Failed to save image to:" << m_outputFile;
        emit finished(false);
    }
}

bool ScreenCapture::captureImagePortal(const QRect &rect, const QString &outputPath)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qWarning() << "DBus session bus not connected";
        return false;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop",
                                                      "/org/freedesktop/portal/desktop",
                                                      "org.freedesktop.portal.Screenshot",
                                                      "Screenshot");

    QVariantMap options;
    options.insert("interactive", false);
    if (rect.isValid() && rect.width() > 0 && rect.height() > 0) {
        QVariantList area;
        area << rect.x() << rect.y() << rect.width() << rect.height();
        options.insert("area", area);
    }

    msg << QString("") << options;

    QDBusPendingCall async = bus.asyncCall(msg);
    QDBusPendingCallWatcher watcher(async);
    QEventLoop callLoop;
    QObject::connect(&watcher, &QDBusPendingCallWatcher::finished, &callLoop, &QEventLoop::quit);
    callLoop.exec();

    QDBusPendingReply<QDBusObjectPath> reply = watcher.reply();
    if (reply.isError()) {
        qWarning() << "Portal Screenshot call failed:" << reply.error().message();
        return false;
    }

    const auto handlePath = reply.value();
    if (handlePath.path().isEmpty()) {
        qWarning() << "Portal returned empty handle";
        return false;
    }

    // Prepare to receive Response
    m_portalSuccess = false;
    m_portalUri.clear();
    QEventLoop responseLoop;
    m_portalEventLoop = &responseLoop;

    bool ok = bus.connect("org.freedesktop.portal.Desktop",
                          handlePath.path(),
                          "org.freedesktop.portal.Request",
                          QStringLiteral("Response"),
                          this,
                          SLOT(onPortalResponse(uint, QVariantMap)));
    if (!ok) {
        qWarning() << "Failed to connect to portal Response signal";
        m_portalEventLoop = nullptr;
        return false;
    }

    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &responseLoop, &QEventLoop::quit);
    timeout.start(30000);
    responseLoop.exec();
    m_portalEventLoop = nullptr;
    bus.disconnect("org.freedesktop.portal.Desktop",
                   handlePath.path(),
                   "org.freedesktop.portal.Request",
                   QStringLiteral("Response"),
                   this,
                   SLOT(onPortalResponse(uint, QVariantMap)));

    if (!m_portalSuccess) {
        qWarning() << "Portal screenshot failed or timed out";
        return false;
    }

    QUrl url(m_portalUri);
    QString srcPath = url.isValid() && url.scheme() == "file" ? url.toLocalFile() : m_portalUri;
    if (srcPath.isEmpty()) {
        qWarning() << "Portal provided empty source path";
        return false;
    }

    if (QFile::exists(outputPath)) {
        QFile::remove(outputPath);
    }
    if (!QFile::copy(srcPath, outputPath)) {
        qWarning() << "Failed to copy portal image from" << srcPath << "to" << outputPath;
        return false;
    }
#endif
    return true;
}

void ScreenCapture::onPortalResponse(uint response, const QVariantMap &results)
{
    if (response == 0) {
        m_portalUri = results.value("uri").toString();
        m_portalSuccess = !m_portalUri.isEmpty();
    } else {
        m_portalSuccess = false;
        qWarning() << "Portal responded with error code" << response;
    }
    if (m_portalEventLoop) {
        m_portalEventLoop->quit();
    }
}

bool ScreenCapture::isWayland()
{
    const QString platformName = QGuiApplication::platformName();
    const QString sessionType = qEnvironmentVariable("XDG_SESSION_TYPE");
    return platformName.contains("wayland", Qt::CaseInsensitive)
           || sessionType.compare("wayland", Qt::CaseInsensitive) == 0;
}

QRect ScreenCapture::adjustRectForVideo(const QRect &rect)
{
    // H.264 encoders require dimensions to be even (divisible by 2)
    // Reduce width/height by 1 if odd
    int width = rect.width();
    int height = rect.height();

    if (width % 2)
        --width;
    if (height % 2)
        --height;

    auto result = QRect(rect.x(), rect.y(), width, height);
    if (result != rect)
        qDebug() << "Adjusted capture area from" << rect << "to" << result
                 << "(H.264 requires even dimensions)";
    return result;
}

QRect ScreenCapture::applyDevicePixelRatio(const QRect &rect)
{
    // Get the device pixel ratio from the primary screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qWarning() << "Could not get screen for device pixel ratio";
        return rect;
    }

    qreal dpr = screen->devicePixelRatio();

    // If DPR is 1.0, no conversion needed
    if (qFuzzyCompare(dpr, 1.0)) {
        return rect;
    }

    int logicalX = qRound(rect.x() * dpr);
    int logicalY = qRound(rect.y() * dpr);
    int logicalWidth = qRound(rect.width() * dpr);
    int logicalHeight = qRound(rect.height() * dpr);

    QRect logicalRect(logicalX, logicalY, logicalWidth, logicalHeight);

    qDebug() << "Device Pixel Ratio:" << dpr;
    qDebug() << "Physical rect (from selector):" << rect;
    qDebug() << "Logical rect (for grabWindow):" << logicalRect;

    return logicalRect;
}

QRect ScreenCapture::invertDevicePixelRatio(const QRect &rect)
{
    // Get the device pixel ratio from the primary screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qWarning() << "Could not get screen for device pixel ratio";
        return rect;
    }

    qreal dpr = screen->devicePixelRatio();

    // If DPR is 1.0, no conversion needed
    if (qFuzzyCompare(dpr, 1.0)) {
        return rect;
    }

    int logicalX = qRound(rect.x() / dpr);
    int logicalY = qRound(rect.y() / dpr);
    int logicalWidth = qRound(rect.width() / dpr);
    int logicalHeight = qRound(rect.height() / dpr);

    QRect logicalRect(logicalX, logicalY, logicalWidth, logicalHeight);

    qDebug() << "Device Pixel Ratio:" << dpr;
    qDebug() << "Physical rect (from selector):" << rect;
    qDebug() << "Logical rect (for grabWindow):" << logicalRect;

    return logicalRect;
}
