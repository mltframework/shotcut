/*
 * Copyright (c) 2012 Meltytech, LLC
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

#ifndef TRANSPORTCONTROL_H
#define TRANSPORTCONTROL_H

#include <QObject>

class TransportControllable : public QObject
{
    Q_OBJECT
public slots:
    virtual void play(double speed = 1.0) = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(int position) = 0;
    virtual void rewind() = 0;
    virtual void fastForward() = 0;
    virtual void previous(int currentPosition) = 0;
    virtual void next(int currentPosition) = 0;
    virtual void setIn(int) = 0;
    virtual void setOut(int) = 0;
};

#endif // TRANSPORTCONTROL_H
