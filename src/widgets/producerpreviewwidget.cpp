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

#include "widgets/producerpreviewwidget.h"

#include <Logger.h>
#include "scrubbar.h"
#include "mltcontroller.h"

#include <QtConcurrent/QtConcurrent>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

ProducerPreviewWidget::ProducerPreviewWidget(double dar)
    : QWidget()
    , m_previewSize(320, 320)
    , m_seekTo(-1)
    , m_timerId(0)
    , m_producer(nullptr)
    , m_queue(10, DataQueue<QueueItem>::OverflowModeWait)
    , m_generateFrames(false)
{
    LOG_DEBUG() << "begin";
    int height = lrint((double)320 / dar);
    height -= height % 2;
    m_previewSize.setHeight( height );

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    m_imageLabel = new QLabel();
    m_imageLabel->setFixedSize(m_previewSize);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_imageLabel);

    m_scrubber = new ScrubBar(this);
    m_scrubber->setFocusPolicy(Qt::NoFocus);
    m_scrubber->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_scrubber->setMinimumWidth(m_previewSize.width());
    m_scrubber->setMargin(0);
    connect(m_scrubber, SIGNAL(seeked(int)), this, SLOT(seeked(int)));
    layout->addWidget(m_scrubber);

    m_posLabel = new QLabel();
    m_posLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_posLabel);

    LOG_DEBUG() << "end";
}

ProducerPreviewWidget::~ProducerPreviewWidget()
{
    stop();
}

void ProducerPreviewWidget::start(Mlt::Producer *producer)
{
    m_producer = producer;

    if ( m_producer ) {
        // Set up the preview display and timer
        m_scrubber->setFramerate(MLT.profile().fps());
        m_scrubber->setScale(m_producer->get_length());
        // Display preview at half frame rate.
        int miliseconds = 2 * 1000.0 / MLT.profile().fps();
        m_timerId = startTimer(miliseconds);
        // Set up the producer frame generator
        m_seekTo = 0;
        m_generateFrames = true;
        m_future = QtConcurrent::run(this, &ProducerPreviewWidget::frameGeneratorThread);
    }
}

void ProducerPreviewWidget::ProducerPreviewWidget::stop()
{
    if ( m_timerId ) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
    m_generateFrames = false;
    while ( m_queue.count() > 0 ) {
        m_queue.pop();
    }
    m_future.waitForFinished();
    if (m_producer) {
        delete m_producer;
        m_producer = nullptr;
    }
    while ( m_queue.count() > 0 ) {
        m_queue.pop();
    }
    m_seekTo = 0;
    m_scrubber->onSeek(0);
    m_scrubber->setScale(0);
    m_posLabel->setText("");
}

void ProducerPreviewWidget::showText(QString text)
{
    m_imageLabel->setText(text);
}

void ProducerPreviewWidget::seeked(int position)
{
    m_seekTo = position;
}

void ProducerPreviewWidget::timerEvent(QTimerEvent *)
{
    if ( m_queue.count() > 0 ) {
        QueueItem item = m_queue.pop();
        m_imageLabel->setPixmap(item.pixmap);
        m_scrubber->onSeek(item.position);
        m_posLabel->setText(item.positionText);
    }
}

void ProducerPreviewWidget::frameGeneratorThread()
{
    while ( m_generateFrames && m_producer ) {
        // Check for seek
        if (m_seekTo != -1) {
            m_producer->seek(m_seekTo);
            m_seekTo = -1;
            while ( m_queue.count() > 1 ) {
                m_queue.pop();
            }
        }
        // Get the image
        int position = m_producer->position();
        int length = m_producer->get_length();
        int width = m_previewSize.width();
        int height = m_previewSize.height();
        mlt_image_format format = mlt_image_rgb;
        Mlt::Frame *frame = m_producer->get_frame();
        frame->set( "rescale.interp", "bilinear" );
        uint8_t *mltImage = frame->get_image( format, width, height, 0 );
        QImage image( mltImage, width, height, QImage::Format_RGB888 );

        // Send the image and status in the queue
        QueueItem item;
        item.pixmap.convertFromImage(image);
        item.position = position;
        item.positionText = QString::fromLatin1(m_producer->frame_time()) + QString(" / ") +
                            QString::fromLatin1(m_producer->get_length_time());
        m_queue.push(item);

        // Seek to the next frame (every other frame with repeat)
        if (position + 2 >= length) {
            m_producer->seek(0);
        } else {
            m_producer->seek(position + 2);
        }
        delete frame;
    }
}
