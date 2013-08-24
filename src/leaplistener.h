/*
 * Copyright (c) 2013 Meltytech, LLC
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

#ifndef LEAPLISTENER_H
#define LEAPLISTENER_H

#include <QObject>
#include <Leap.h>

class LeapListener : public QObject, Leap::Listener
{
    Q_OBJECT
public:
    explicit LeapListener(QObject *parent = 0);
    virtual ~LeapListener();

signals:
    void keyTap();
    void screenTap();
    void jogRightFrame();
    void jogLeftFrame();
    void jogRightSecond();
    void jogLeftSecond();
    void shuttle(float);

private:
    Leap::Controller m_controller;
    Leap::Frame m_lastFrame;

    void onConnect(const Leap::Controller& controller);
    void onFocusGained(const Leap::Controller& controller);
    void onFrame(const Leap::Controller& controller);
};

#endif // LEAPLISTENER_H
