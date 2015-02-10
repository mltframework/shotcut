/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#include "audiowaveformscopewidget.h"
#include <QDebug>
#include <QPainter>
#include <QResizeEvent>
#include <MltProducer.h>
#include <MltProfile.h>

AudioWaveformScopeWidget::AudioWaveformScopeWidget()
  : ScopeWidget("AudioWaveform")
  , m_frame()
  , m_filter(0)
  , m_prevSize(0, 0)
  , m_renderWave()
  , m_refreshTime()
  , m_mutex(QMutex::NonRecursive)
  , m_displayWave()
  , m_size(0, 0)
{
    qDebug() << "begin";
    m_refreshTime.start();
    qDebug() << "end";
}

AudioWaveformScopeWidget::~AudioWaveformScopeWidget()
{
    qDebug() << "begin";
    delete m_filter;
    qDebug() << "end";
}

void AudioWaveformScopeWidget::refreshScope()
{
    static const qreal MAX_AMPLITUDE = 32768.0;

    m_mutex.lock();
    QSize currentSize = m_size;
    m_mutex.unlock();

    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    if (m_prevSize == currentSize && m_refreshTime.elapsed() < 90) {
        // When not resizing, limit refreshes to 90ms.
        return;
    }

    if (m_renderWave.size() != currentSize) {
        m_renderWave = QImage(currentSize.width(), currentSize.height(), QImage::Format_ARGB32_Premultiplied);
    }
    QColor bgColor( 0, 0, 0 ,0 );
    m_renderWave.fill(bgColor);

    QPainter p(&m_renderWave);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::white);

    if (m_frame.is_valid() && m_frame.get_audio_samples() > 0) {
        int channels = m_frame.get_audio_channels();
        int samples = m_frame.get_audio_samples();
        int16_t* audio = (int16_t*)m_frame.get_audio();
        int waveAmplitude = currentSize.height() / 4;
        qreal scaleFactor = (qreal)waveAmplitude / (qreal)MAX_AMPLITUDE;

        for (int c = 0; c < channels; c++)
        {
            p.save();
            p.translate(0, waveAmplitude + (2 * c* waveAmplitude));

            // For each x position on the waveform, find the min and max sample
            // values that apply to that position. Draw a vertical line from the
            // min value to the max value.
            QPoint high;
            QPoint low;
            int lastX = 0;
            const int16_t* q = audio + c;
            qreal max = *q;
            qreal min = *q;

            for (int i = 0; i <= samples; i++)
            {
                int x = ( i * currentSize.width() ) / samples;
                if (x != lastX) {
                    // The min and max have been determined for the previous x
                    // So draw the line
                    high.setX(lastX);
                    high.setY(max * scaleFactor);
                    low.setX(lastX);
                    low.setY(min * scaleFactor);
                    if (high.y() == low.y()) {
                        p.drawPoint(high);
                    } else {
                        p.drawLine(low, high);
                    }
                    lastX = x;

                    // Swap max and min so that the next line picks up where
                    // this one left off.
                    int tmp = max;
                    max = min;
                    min = tmp;
                }

                if (*q > max) max = *q;
                if (*q < min) min = *q;
                q += channels;
            }
            p.restore();
        }
    }

    p.end();

    m_mutex.lock();
    m_displayWave.swap(m_renderWave);
    m_mutex.unlock();

    m_prevSize = currentSize;
    m_refreshTime.restart();
}

void AudioWaveformScopeWidget::paintEvent(QPaintEvent*)
{
    if (!isVisible())
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(0, 0, width(), height(), QBrush(Qt::black, Qt::SolidPattern));
    m_mutex.lock();
    p.drawImage(rect(), m_displayWave, m_displayWave.rect());
    m_mutex.unlock();
    p.end();
}

void AudioWaveformScopeWidget::resizeEvent(QResizeEvent*)
{
    m_mutex.lock();
    m_size = size();
    m_mutex.unlock();
    requestRefresh();
}

QString AudioWaveformScopeWidget::getTitle()
{
   return tr("Audio Waveform");
}
