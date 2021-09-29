/*
 * Copyright (c) 2015-2016 Meltytech, LLC
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

#include "audiopeakmeterscopewidget.h"
#include "settings.h"
#include <Logger.h>
#include <QVBoxLayout>
#include <MltProfile.h>
#include "widgets/audiometerwidget.h"
#include "mltcontroller.h"
#include <cmath> // log10()

AudioPeakMeterScopeWidget::AudioPeakMeterScopeWidget()
  : ScopeWidget("AudioPeakMeter")
  , m_filter(0)
  , m_audioMeter(0)
  , m_orientation((Qt::Orientation)-1)
  , m_channels( Settings.playerAudioChannels() )
{
    LOG_DEBUG() << "begin";
    m_filter = new Mlt::Filter(MLT.profile(), "audiolevel");
    m_filter->set("iec_scale", 0);
    qRegisterMetaType< QVector<double> >("QVector<double>");
    setAutoFillBackground(true);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(4, 4, 4, 4);
    m_audioMeter = new AudioMeterWidget(this);
    m_audioMeter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVector<int> dbscale;
    dbscale << -50 << -40 << -35 << -30 << -25 << -20 << -15 << -10 << -5 << 0 << 3;
    m_audioMeter->setDbLabels(dbscale);
    vlayout->addWidget(m_audioMeter);
    LOG_DEBUG() << "end";
}

AudioPeakMeterScopeWidget::~AudioPeakMeterScopeWidget()
{
    delete m_filter;
}

void AudioPeakMeterScopeWidget::refreshScope(const QSize& /*size*/, bool /*full*/)
{
    SharedFrame sFrame;
    while (m_queue.count() > 0) {
        sFrame = m_queue.pop();
        if (sFrame.is_valid() && sFrame.get_audio_samples() > 0) {
            mlt_audio_format format = mlt_audio_s16;
            int channels = sFrame.get_audio_channels();
            int frequency = sFrame.get_audio_frequency();
            int samples = sFrame.get_audio_samples();
            Mlt::Frame mFrame = sFrame.clone(true, false, false);
            m_filter->process(mFrame);
            mFrame.get_audio( format, frequency, channels, samples );
            QVector<double> levels;
            for (int i = 0; i < channels; i++) {
                QString s = QString("meta.media.audio_level.%1").arg(i);
                double audioLevel = mFrame.get_double(s.toLatin1().constData());
                if (audioLevel == 0.0) {
                    levels << -100.0;
                } else {
                    levels << 20 * log10(audioLevel);
                }
            }
            QMetaObject::invokeMethod(m_audioMeter, "showAudio", Qt::QueuedConnection, Q_ARG(const QVector<double>&, levels));
            if (m_channels != channels) {
                m_channels = channels;
                QMetaObject::invokeMethod(this, "reconfigureMeter", Qt::QueuedConnection);
            }
        }
    }
}

QString AudioPeakMeterScopeWidget::getTitle()
{
   return tr("Audio Peak Meter");
}

void AudioPeakMeterScopeWidget::setOrientation(Qt::Orientation orientation)
{
    if (orientation != m_orientation) {
        m_orientation = orientation;
        m_audioMeter->setOrientation(orientation);
        reconfigureMeter();
    }
}

void AudioPeakMeterScopeWidget::reconfigureMeter()
{
    // Set the bar labels.
    QStringList channelLabels;
    if (m_channels == 2 )
        channelLabels << tr("L") << tr("R");
    else if (m_channels == 6 )
        channelLabels << tr("L") << tr("R") << tr("C") << tr("LF") << tr("Ls") << tr("Rs");
    m_audioMeter->setChannelLabels(channelLabels);

    // Set the size constraints.
    int spaceNeeded = ( m_channels * 16 ) + 17;
    if (m_orientation == Qt::Vertical) {
        m_audioMeter->setMinimumSize(spaceNeeded, 250);
        setMinimumSize(spaceNeeded + 8, 258);
        setMaximumSize(spaceNeeded + 8, 508);
    } else {
        m_audioMeter->setMinimumSize(250, spaceNeeded);
        setMinimumSize(258, spaceNeeded + 8);
        setMaximumSize(508, spaceNeeded + 8);
    }
    updateGeometry();
}
