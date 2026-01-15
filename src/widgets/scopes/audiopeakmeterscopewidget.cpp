/*
 * Copyright (c) 2015-2026 Meltytech, LLC
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

#include "Logger.h"
#include "mltcontroller.h"
#include "settings.h"
#include "widgets/audiometerwidget.h"

#include <QVBoxLayout>

#include <cmath> // log10()

AudioPeakMeterScopeWidget::AudioPeakMeterScopeWidget()
    : ScopeWidget("AudioPeakMeter")
    , m_audioMeter(0)
    , m_orientation((Qt::Orientation) -1)
    , m_channels(Settings.playerAudioChannels())
{
    LOG_DEBUG() << "begin";
    qRegisterMetaType<QVector<double>>("QVector<double>");
    setAutoFillBackground(true);
    setWhatsThis("https://forum.shotcut.org/t/audio-peak-meter-scope/12918/1");
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(4, 4, 4, 4);
    m_audioMeter = new AudioMeterWidget(this);
    m_audioMeter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVector<int> dbscale;
    dbscale << -50 << -40 << -35 << -30 << -25 << -20 << -15 << -10 << -5 << 0;
    m_audioMeter->setDbLabels(dbscale);
    vlayout->addWidget(m_audioMeter);
    LOG_DEBUG() << "end";
}

void AudioPeakMeterScopeWidget::refreshScope(const QSize & /*size*/, bool /*full*/)
{
    SharedFrame sFrame;
    while (m_queue.count() > 0) {
        sFrame = m_queue.pop();
        if (sFrame.is_valid() && sFrame.get_audio_samples() > 0) {
            int channels = sFrame.get_audio_channels();
            int samples = sFrame.get_audio_samples();
            QVector<double> levels;
            const int16_t *audio = sFrame.get_audio();
            for (int c = 0; c < channels; c++) {
                int16_t peak = 0;
                const int16_t *p = audio + c;
                for (int s = 0; s < samples; s++) {
                    int16_t sample = abs(*p);
                    if (sample > peak)
                        peak = sample;
                    p += channels;
                }
                if (peak == 0) {
                    levels << -100.0;
                } else {
                    levels << 20
                                  * log10((double) peak
                                          / (double) std::numeric_limits<int16_t>::max());
                }
            }
            QMetaObject::invokeMethod(m_audioMeter,
                                      "showAudio",
                                      Qt::QueuedConnection,
                                      Q_ARG(const QVector<double> &, levels));
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
    if (m_channels == 2)
        channelLabels << tr("L") << tr("R");
    if (m_channels == 4)
        channelLabels << tr("L") << tr("R") << tr("Ls") << tr("Rs");
    else if (m_channels == 6)
        channelLabels << tr("L") << tr("R") << tr("C") << tr("LF") << tr("Ls") << tr("Rs");
    m_audioMeter->setChannelLabels(channelLabels);

    // Set the size constraints.
    int spaceNeeded = (m_channels * 16) + 17;
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
