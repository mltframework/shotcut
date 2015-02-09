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
        Mlt::Frame resampledFrame = m_frame.clone(true, false, false);

        if (currentSize == m_prevSize) {
            delete m_filter;
            m_filter = 0;
        }

        if (!m_filter) {
            Mlt::Profile profile;
            m_filter = new Mlt::Filter(profile, "resample");
        }
        m_filter->process(resampledFrame);
        mlt_audio_format format = m_frame.get_audio_format();
        int scaledFrequency = (m_frame.get_audio_frequency() * currentSize.width()) / m_frame.get_audio_samples();
        int scaledSamples = 0;
        int16_t* scaledAudio = (int16_t*)resampledFrame.get_audio(format, scaledFrequency, channels, scaledSamples);

        scaledSamples = qMin(scaledSamples, currentSize.width());

        int waveAmplitude = currentSize.height() / 4;
        qreal scaleFactor = (qreal)waveAmplitude / (qreal)MAX_AMPLITUDE;

        for (int c = 0; c < channels; c++)
        {
            p.save();
            p.translate(0, waveAmplitude + (2 * c* waveAmplitude));
            const int16_t* q = scaledAudio + c;
            QPoint point(0, (qreal)*q * scaleFactor);
            p.drawPoint(point);
            QPoint prevPoint = point;

            for (int i = 1; i < scaledSamples; i++)
            {
                q += channels;
                point.setX(i);
                point.setY((qreal)*q * scaleFactor);
                if (qAbs( point.y() - prevPoint.y() ) < 2) {
                    p.drawPoint(point);
                } else {
                    p.drawLine(prevPoint, point);
                }
                prevPoint = point;
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
