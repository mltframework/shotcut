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

#ifndef HTMLGENERATOR_H
#define HTMLGENERATOR_H

#include <QElapsedTimer>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QSize>
#include <QTemporaryDir>
#include <QTimer>
#include <QWebSocket>

class HtmlGenerator : public QObject
{
    Q_OBJECT
public:
    explicit HtmlGenerator(QObject *parent = nullptr);
    ~HtmlGenerator();

    void setAnimationParameters(double fps, int duration);
    void launchBrowser(const QString &executablePath,
                       const QString &url,
                       const QSize &viewport,
                       const QString &outputPath);

signals:
    void progressUpdate(float);
    void imageReady(QString outputPath);

private slots:
    void connectToBrowser();
    void createNewPage();
    void onWebSocketConnected();
    void onMessageReceived(const QString &message);
    void onWebSocketDisconnected();
    void onChromeProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onChromeProcessError(QProcess::ProcessError error);
    void startAnimationCapture();
    void captureAnimationFrame();
    void handleAnimationFrame(const QJsonObject &result);
    void completeAnimationCapture();
    void takeScreenshot();
    void handleScreenshotResult(const QJsonObject &result);
    int sendCommand(const QString &method, const QJsonObject &params = QJsonObject());

private:
    QWebSocket *m_webSocket;
    QNetworkAccessManager *m_networkManager;
    int m_messageId;
    QProcess *m_chromeProcess;
    QString m_url;
    QSize m_viewport;
    QString m_outputPath;
    bool m_pendingScreenshot = false;
    int m_screenshotMessageId = 0;
    bool m_screenshotCompleted = false;
    QTemporaryDir m_tempDir;

    // Animation recording members
    bool m_animationMode = false;
    double m_fps = 0.0;
    int m_duration = 0;
    int m_currentFrame = 0;
    int m_totalFrames = 0;
    QTimer *m_animationTimer = nullptr;
    QElapsedTimer m_animationElapsed;
};

#endif // HTMLGENERATOR_H
