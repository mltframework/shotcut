/*
 * Copyright (c) 2020 Meltytech, LLC
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

#ifndef PRODUCERPREVIEWWIDGET_H
#define PRODUCERPREVIEWWIDGET_H

#include "dataqueue.h"

#include <QFuture>
#include <QPixmap>
#include <QWidget>

class QLabel;
class ScrubBar;
namespace Mlt {
class Producer;
}

class ProducerPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProducerPreviewWidget(double dar);
    virtual ~ProducerPreviewWidget();

    void start(Mlt::Producer *producer);
    void stop();
    void showText(QString text);

private slots:
    void seeked(int);

private:
    void timerEvent(QTimerEvent *) override;
    void frameGeneratorThread();

    QSize m_previewSize;
    QLabel *m_imageLabel;
    ScrubBar *m_scrubber;
    QLabel *m_posLabel;
    int m_seekTo;
    int m_timerId;
    Mlt::Producer *m_producer;
    struct QueueItem {
        QPixmap pixmap;
        int position;
        QString positionText;
    };
    DataQueue<QueueItem> m_queue;
    QFuture<void> m_future;
    bool m_generateFrames;
};

#endif // PRODUCERPREVIEWWIDGET_H
