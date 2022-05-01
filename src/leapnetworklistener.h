/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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

#ifndef LEAPNETWORKLISTENER_H
#define LEAPNETWORKLISTENER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QWebSocket>

class LeapNetworkListener : public QObject
{
    Q_OBJECT
public:
    explicit LeapNetworkListener(QObject *parent = 0);

    void start();

signals:
    void jogRightFrame();
    void jogLeftFrame();
    void jogRightSecond();
    void jogLeftSecond();
    void shuttle(float);

private slots:
    void onConnected();
    void onDisconnected();
    void heartbeat();
    void onMessage(const QString &s);
    void onError(QAbstractSocket::SocketError error);

private:
    QWebSocket m_socket;
    QTimer m_timer;
};

#endif // LEAPNETWORKLISTENER_H
