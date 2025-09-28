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

#include "htmlgenerator.h"
#include "Logger.h"
#include "settings.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrl>

HtmlGenerator::HtmlGenerator(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_messageId(1)
    , m_chromeProcess(new QProcess(this))
{
    connect(m_webSocket, &QWebSocket::connected, this, &HtmlGenerator::onWebSocketConnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &HtmlGenerator::onMessageReceived);
    connect(m_webSocket, &QWebSocket::disconnected, this, &HtmlGenerator::onWebSocketDisconnected);
}

HtmlGenerator::~HtmlGenerator()
{
    if (m_chromeProcess && m_chromeProcess->state() == QProcess::Running) {
        m_chromeProcess->terminate();
        m_chromeProcess->waitForFinished(2000);
        m_chromeProcess->kill();
    }
}

void HtmlGenerator::setAnimationParameters(double fps, int duration)
{
    m_fps = fps;
    m_duration = duration;
    m_animationMode = (fps > 0 && duration > 0);
}

void HtmlGenerator::launchBrowser(const QString &executablePath,
                                  const QString &url,
                                  const QSize &viewport,
                                  const QString &outputPath)
{
    m_url = url;
    m_viewport = viewport;
    m_outputPath = outputPath;

    // Start browser with appropriate arguments
    QStringList arguments;
    arguments << "--remote-debugging-port=9222"
              << "--headless=new"
              << "--disable-gpu"
              << "--no-sandbox"
              << "--no-zygote"
              // << "--single-process"
              << "--no-first-run"
              << "--no-default-browser-check"
              << "--allow-file-access-from-files";

    connect(m_chromeProcess, &QProcess::finished, this, &HtmlGenerator::onChromeProcessFinished);
    connect(m_chromeProcess, &QProcess::errorOccurred, this, &HtmlGenerator::onChromeProcessError);

    m_chromeProcess->start(executablePath, arguments);

    if (!m_chromeProcess->waitForStarted(5000)) {
        LOG_ERROR() << "Failed to start browser process";
        Settings.setChromiumPath(QString());
        return;
    }

    // Wait a bit for browser to start up
    QTimer::singleShot(2000, this, &HtmlGenerator::connectToBrowser);
}

void HtmlGenerator::connectToBrowser()
{
    // Get the list of pages from Chrome
    QNetworkRequest request(QUrl("http://localhost:9222/json/list"));
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR() << "Failed to get Chrome debug info:" << reply->errorString();
            Settings.setChromiumPath(QString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray pages = doc.array();

        QString webSocketUrl;
        // Look for an existing page or create a new one
        for (auto pageValue : std::as_const(pages)) {
            const auto page = pageValue.toObject();
            if (page["type"].toString() == "page") {
                webSocketUrl = page["webSocketDebuggerUrl"].toString();
                break;
            }
        }

        if (webSocketUrl.isEmpty()) {
            // Create a new page
            LOG_DEBUG() << "No existing page found, creating a new one";
            createNewPage();
        } else {
            LOG_DEBUG() << "Using existing page, connecting to WebSocket:" << webSocketUrl;
            m_webSocket->open(QUrl(webSocketUrl));
        }
        reply->deleteLater();
    });
}

void HtmlGenerator::createNewPage()
{
    QNetworkRequest request(QUrl("http://localhost:9222/json/new"));
    auto *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR() << "Failed to create new page:" << reply->errorString();
            return;
        }

        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto page = doc.object();
        const auto webSocketUrl = page["webSocketDebuggerUrl"].toString();

        if (webSocketUrl.isEmpty()) {
            LOG_ERROR() << "No WebSocket URL found in new page response";
            return;
        }

        LOG_DEBUG() << "Created new page, connecting to WebSocket:" << webSocketUrl;
        m_webSocket->open(QUrl(webSocketUrl));
        reply->deleteLater();
    });
}

void HtmlGenerator::onWebSocketConnected()
{
    LOG_DEBUG() << "Connected to Chrome DevTools";

    // Enable Runtime and Page events
    sendCommand("Runtime.enable");
    sendCommand("Page.enable");

    // Set default background color to transparent
    QJsonObject params;
    params["color"] = QJsonObject{{"r", 0}, {"g", 0}, {"b", 0}, {"a", 0}};
    sendCommand("Emulation.setDefaultBackgroundColorOverride", params);

    // Set viewport
    params = QJsonObject();
    params["width"] = m_viewport.width();
    params["height"] = m_viewport.height();
    params["deviceScaleFactor"] = 1;
    params["mobile"] = false;
    sendCommand("Emulation.setDeviceMetricsOverride", params);

    // Navigate to the URL
    QJsonObject navParams;
    navParams["url"] = m_url;
    sendCommand("Page.navigate", navParams);

    // Set a timeout for taking screenshot in case page load event doesn't fire
    QTimer::singleShot(10000, this, &HtmlGenerator::takeScreenshot);
}

void HtmlGenerator::onMessageReceived(const QString &message)
{
    // if (!message.contains("\"data\""))
    //     LOG_DEBUG() << "Received message:" << message;

    const auto doc = QJsonDocument::fromJson(message.toUtf8());
    const auto obj = doc.object();

    if (obj.contains("method")) {
        const auto method = obj["method"].toString();
        if (method == "Page.loadEventFired") {
            LOG_DEBUG() << "Page loaded, starting capture";
            if (m_animationMode) {
                startAnimationCapture();
            } else {
                takeScreenshot();
            }
        }
    } else if (obj.contains("id") && obj.contains("result")) {
        const auto id = obj["id"].toInt();
        if (m_pendingScreenshot && id == m_screenshotMessageId) {
            if (m_animationMode) {
                handleAnimationFrame(obj["result"].toObject());
            } else {
                handleScreenshotResult(obj["result"].toObject());
            }
        }
    } else if (obj.contains("error")) {
        LOG_ERROR() << "Error received:" << QJsonDocument(obj["error"].toObject()).toJson();
    }
}

void HtmlGenerator::onWebSocketDisconnected()
{
    LOG_DEBUG() << "Disconnected from Chrome DevTools";
}

void HtmlGenerator::onChromeProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    LOG_DEBUG() << "Chrome process finished" << exitCode;
}

void HtmlGenerator::onChromeProcessError(QProcess::ProcessError error)
{
    // Ignore crash errors after successful screenshot completion
    if (error == QProcess::Crashed && m_screenshotCompleted) {
        LOG_DEBUG() << "Chrome process crashed after successful screenshot completion (expected)";
        return;
    }

    LOG_ERROR() << "Chrome process error:" << error;
}

void HtmlGenerator::startAnimationCapture()
{
    // Create output directory
    const QDir outputDir(m_outputPath);
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(".")) {
            LOG_ERROR() << "Failed to create output directory:" << m_outputPath;
            return;
        }
    }

    // Calculate animation parameters
    const auto frameInterval = 1000.0 / m_fps; // milliseconds per frame
    m_totalFrames = static_cast<int>(std::ceil(m_duration / frameInterval));
    m_currentFrame = 0;

    LOG_DEBUG() << QString("Capturing %1 frames at %2 fps over %3ms...")
                       .arg(m_totalFrames)
                       .arg(m_fps)
                       .arg(m_duration);

    // Start timing
    m_animationElapsed.start();

    // Take first frame
    captureAnimationFrame();
}

void HtmlGenerator::captureAnimationFrame()
{
    if (m_currentFrame >= m_totalFrames) {
        completeAnimationCapture();
        return;
    }

    // LOG_DEBUG() << "Taking screenshot...";
    m_pendingScreenshot = true;

    QJsonObject params;
    params["format"] = "png";
    params["omitBackground"] = true;
    params["captureBeyondViewport"] = false;
    m_screenshotMessageId = sendCommand("Page.captureScreenshot", params);
}

void HtmlGenerator::handleAnimationFrame(const QJsonObject &result)
{
    m_pendingScreenshot = false;

    const auto base64Data = result["data"].toString();
    const auto imageData = QByteArray::fromBase64(base64Data.toUtf8());

    // Save frame with zero-padded filename
    const auto frameNumber = QString("%1").arg(m_currentFrame, 4, 10, QLatin1Char('0'));
    const auto filename = QDir(m_outputPath).filePath(QString("frame_%1.png").arg(frameNumber));

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(imageData);
        file.close();
        LOG_DEBUG() << "Captured frame" << (m_currentFrame + 1) << "/" << m_totalFrames;
        emit progressUpdate(float(m_currentFrame + 1) / m_totalFrames);
    } else {
        LOG_ERROR() << "Failed to save frame:" << file.errorString();
        return;
    }

    m_currentFrame++;

    // Schedule next frame
    if (m_currentFrame < m_totalFrames) {
        // Calculate when the next frame should be captured
        const auto frameInterval = 1000.0 / m_fps;
        const auto targetTime = static_cast<qint64>(m_currentFrame * frameInterval);
        const auto currentTime = m_animationElapsed.elapsed();

        int delay = std::max(0LL, targetTime - currentTime);
        if (delay == 0)
            LOG_DEBUG() << "frame duration" << frameInterval << "delay" << targetTime - currentTime
                        << "ms";
        QTimer::singleShot(delay, this, &HtmlGenerator::captureAnimationFrame);
    } else {
        completeAnimationCapture();
    }
}

void HtmlGenerator::completeAnimationCapture()
{
    m_screenshotCompleted = true;

    LOG_DEBUG() << QString("Animation frames saved to %1/").arg(m_outputPath);

    // Close the browser and exit
    m_webSocket->close();
    if (m_chromeProcess && m_chromeProcess->state() == QProcess::Running) {
        m_chromeProcess->terminate();
    }

    // Emit signal indicating animation frames are ready
    emit imageReady(m_outputPath);
}

void HtmlGenerator::takeScreenshot()
{
    if (m_pendingScreenshot) {
        LOG_DEBUG() << "Screenshot already pending, skipping";
        return;
    }

    LOG_DEBUG() << "Taking screenshot...";
    m_pendingScreenshot = true;

    QJsonObject params;
    params["format"] = "png";
    params["omitBackground"] = true;

    m_screenshotMessageId = sendCommand("Page.captureScreenshot", params);
}

void HtmlGenerator::handleScreenshotResult(const QJsonObject &result)
{
    m_pendingScreenshot = false;
    m_screenshotCompleted = true;

    const auto base64Data = result["data"].toString();
    const auto imageData = QByteArray::fromBase64(base64Data.toUtf8());
    bool success = false;

    QFile file(m_outputPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(imageData);
        file.close();
        success = true;
        LOG_DEBUG() << "Screenshot saved to:" << m_outputPath;
    } else {
        LOG_ERROR() << "Failed to save screenshot:" << file.errorString();
    }

    // Close the browser and exit
    sendCommand("Browser.close");
    QTimer::singleShot(100, m_chromeProcess, [=]() {
        m_webSocket->close();
        if (m_chromeProcess && m_chromeProcess->state() == QProcess::Running)
            m_chromeProcess->terminate();
    });
    if (success)
        emit imageReady(m_outputPath);
}

int HtmlGenerator::sendCommand(const QString &method, const QJsonObject &params)
{
    QJsonObject command;
    command["id"] = m_messageId;
    command["method"] = method;
    if (!params.isEmpty()) {
        command["params"] = params;
    }

    const QJsonDocument doc(command);
    const QString message = doc.toJson(QJsonDocument::Compact);

    // LOG_DEBUG() << "Sending command:" << message;
    m_webSocket->sendTextMessage(message);

    return m_messageId++;
}
