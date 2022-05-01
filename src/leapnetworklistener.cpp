/*
 * Copyright (c) 2013-2016 Meltytech, LLC
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

#include "leapnetworklistener.h"
#include <Logger.h>
#include <QtCore>

static const float BIG_CIRCLE_THRESHOLD = 70.0f;

LeapNetworkListener::LeapNetworkListener(QObject *parent) :
    QObject(parent)
{
    start();
}

void LeapNetworkListener::start()
{
    LOG_DEBUG() << "begin";
    QUrl url("ws://localhost:6437/v2.json");
    connect(&m_socket, SIGNAL(connected()), SLOT(onConnected()));
    connect(&m_socket, SIGNAL(disconnected()), SLOT(onDisconnected()));
    connect(&m_socket, SIGNAL(textFrameReceived(QString, bool)), SLOT(onMessage(QString)));
    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(onError(QAbstractSocket::SocketError)));
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(heartbeat()));
    m_socket.open(url);
}

void LeapNetworkListener::onConnected()
{
    LOG_DEBUG() << "Connected to Leap Motion";
    m_socket.sendTextMessage("{\"enableGestures\": true}");
}

void LeapNetworkListener::onDisconnected()
{
    LOG_DEBUG() << "Disconnected from Leap Motion";
    m_timer.stop();
}

void LeapNetworkListener::heartbeat()
{
    m_socket.sendTextMessage("{\"heartbeat\": true}");
}

void LeapNetworkListener::onMessage(const QString &s)
{
    QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
    if (!doc.isNull()) {
        QJsonObject frame = doc.object();
        if (frame["gestures"].isArray() && frame["gestures"].toArray().count() > 0) {
            foreach (QJsonValue jv, frame["gestures"].toArray()) {
                QJsonObject gesture = jv.toObject();
                if (gesture["type"].toString() == "circle") {
                    bool isClockwise = (gesture["normal"].toArray()[2].toDouble() <= 0);
                    double radius = gesture["radius"].toDouble();
                    if (isClockwise) {
                        if (radius < BIG_CIRCLE_THRESHOLD)
                            emit jogRightFrame();
                        else
                            emit jogRightSecond();
                    } else {
                        if (radius < BIG_CIRCLE_THRESHOLD)
                            emit jogLeftFrame();
                        else
                            emit jogLeftSecond();
                    }
                }
            }
        } else if (frame["hands"].isArray() && frame["hands"].toArray().count() > 0) {
            QJsonObject hand = frame["hands"].toArray().first().toObject();
            if (frame["pointables"].toArray().count() > 1) {
                if (hand["palmPosition"].isArray() && frame["interactionBox"].isObject()) {
                    double x = hand["palmPosition"].toArray().first().toDouble();
                    QJsonValue center = frame["interactionBox"].toObject()["center"];
                    QJsonValue size = frame["interactionBox"].toObject()["size"];
                    if (center.isArray() && size.isArray()) {
                        double normalized = ((x - center.toArray()[0].toDouble()) / size.toArray()[0].toDouble()) + 0.5;
                        // clamp [0, 1]
                        normalized = qBound(0.0, normalized, 1.0);
                        emit shuttle(2.0f * (normalized - 0.5));
                    }
                }
            } else {
                emit shuttle(0);
            }
        } else if (!frame["version"].isNull() && frame["version"].toDouble() >= 2.0) {
            m_timer.start(90);
        }
    }
}

void LeapNetworkListener::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    LOG_DEBUG() << "Leap Motion WebSocket error:" << m_socket.errorString();
}
