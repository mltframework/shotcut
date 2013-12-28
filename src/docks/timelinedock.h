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

#ifndef TIMELINEDOCK_H
#define TIMELINEDOCK_H

#include <QDockWidget>
#include <QQuickView>
#include "models/multitrackmodel.h"
#include "mltcontroller.h"

namespace Ui {
class TimelineDock;
}

class TimelineDock : public QDockWidget
{
    Q_OBJECT
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged);

public:
    explicit TimelineDock(QWidget *parent = 0);
    ~TimelineDock();

    MultitrackModel* model() { return &m_model; }
    int position() const { return m_position; }
    void setPosition(int position);
    Q_INVOKABLE QString timecode(int frames);
    Mlt::ClipInfo* getClipInfo(int trackIndex, int clipIndex);
    Mlt::Producer* getClip(int trackIndex, int clipIndex);

signals:
    void seeked(int position);
    void positionChanged();
    void clipOpened(void* producer, int in, int out);
    void dragging(const QPointF& pos, int duration);
    void dropped();

public slots:
    void addAudioTrack();
    void addVideoTrack();
    void close();
    void onShowFrame(Mlt::QFrame frame);
    void onSeeked(int position);
    void append(int trackIndex);
    void remove(int trackIndex, int clipIndex);
    void lift(int trackIndex, int clipIndex);
    void pressKey(int key, Qt::KeyboardModifiers modifiers);
    void releaseKey(int key, Qt::KeyboardModifiers modifiers);
    void selectTrack(int by);
    void openClip(int trackIndex, int clipIndex);
    void selectClip(int trackIndex, int clipIndex);
    void setTrackName(int trackIndex, const QString& value);
    void toggleTrackMute(int trackIndex);

protected:
    void dragEnterEvent(QDragEnterEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dragLeaveEvent(QDragLeaveEvent* event);
    void dropEvent(QDropEvent* event);

private:
    Ui::TimelineDock *ui;
    QQuickView m_quickView;
    MultitrackModel m_model;
    int m_position;
};

#endif // TIMELINEDOCK_H
