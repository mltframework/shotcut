/*
 * Copyright (c) 2015-2016 Meltytech, LLC
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

#include "audiospectrumscopewidget.h"

#include "Logger.h"
#include "widgets/audiometerwidget.h"

#include <MltProfile.h>
#include <QPainter>
#include <QVBoxLayout>
#include <QtAlgorithms>

#include <cmath>

static const int WINDOW_SIZE = 8000; // 6 Hz FFT bins at 48kHz

struct band
{
    float low;    // Low frequency
    float center; // Center frequency
    float high;   // High frequency
    const char *label;
};

// Preferred frequencies from ISO R 266-1997 / ANSI S1.6-1984
static const band BAND_TAB[] = {
    //     Low      Preferred  High                Band
    //     Freq      Center    Freq     Label       Num
    {1.12, 1.25, 1.41, "1.25"},            //  1
    {1.41, 1.60, 1.78, "1.6"},             //  2
    {1.78, 2.00, 2.24, "2.0"},             //  3
    {2.24, 2.50, 2.82, "2.5"},             //  4
    {2.82, 3.15, 3.55, "3.15"},            //  5
    {3.55, 4.00, 4.44, "4.0"},             //  6
    {4.44, 5.00, 6.00, "5.0"},             //  7
    {6.00, 6.30, 7.00, "6.3"},             //  8
    {7.00, 8.00, 9.00, "8.0"},             //  9
    {9.00, 10.00, 11.00, "10"},            // 10
    {11.00, 12.50, 14.00, "12.5"},         // 11
    {14.00, 16.00, 18.00, "16"},           // 12
    {18.00, 20.00, 22.00, "20"},           // 13 - First in audible range
    {22.00, 25.00, 28.00, "25"},           // 14
    {28.00, 31.50, 35.00, "31"},           // 15
    {35.00, 40.00, 45.00, "40"},           // 16
    {45.00, 50.00, 56.00, "50"},           // 17
    {56.00, 63.00, 71.00, "63"},           // 18
    {71.00, 80.00, 90.00, "80"},           // 19
    {90.00, 100.00, 112.00, "100"},        // 20
    {112.00, 125.00, 140.00, "125"},       // 21
    {140.00, 160.00, 179.00, "160"},       // 22
    {179.00, 200.00, 224.00, "200"},       // 23
    {224.00, 250.00, 282.00, "250"},       // 24
    {282.00, 315.00, 353.00, "315"},       // 25
    {353.00, 400.00, 484.00, "400"},       // 26
    {484.00, 500.00, 560.00, "500"},       // 27
    {560.00, 630.00, 706.00, "630"},       // 28
    {706.00, 800.00, 897.00, "800"},       // 29
    {897.00, 1000.00, 1121.00, "1k"},      // 30
    {1121.00, 1250.00, 1401.00, "1.3k"},   // 31
    {1401.00, 1600.00, 1794.00, "1.6k"},   // 32
    {1794.00, 2000.00, 2242.00, "2k"},     // 33
    {2242.00, 2500.00, 2803.00, "2.5k"},   // 34
    {2803.00, 3150.00, 3531.00, "3.2k"},   // 35
    {3531.00, 4000.00, 4484.00, "4k"},     // 36
    {4484.00, 5000.00, 5605.00, "5k"},     // 37
    {5605.00, 6300.00, 7062.00, "6.3k"},   // 38
    {7062.00, 8000.00, 8908.00, "8k"},     // 39
    {8908.00, 10000.00, 11210.00, "10k"},  // 40
    {11210.00, 12500.00, 14012.00, "13k"}, // 41
    {14012.00, 16000.00, 17936.00, "16k"}, // 42
    {17936.00, 20000.00, 22421.00, "20k"}, // 43 - Last in audible range
};

static const int FIRST_AUDIBLE_BAND_INDEX = 12;
static const int LAST_AUDIBLE_BAND_INDEX = 42;
static const int AUDIBLE_BAND_COUNT = LAST_AUDIBLE_BAND_INDEX - FIRST_AUDIBLE_BAND_INDEX + 1;

AudioSpectrumScopeWidget::AudioSpectrumScopeWidget()
    : ScopeWidget("AudioSpectrum")
    , m_audioMeter(0)
{
    LOG_DEBUG() << "begin";

    // Setup this widget
    qRegisterMetaType<QVector<double>>("QVector<double>");

    // Create the FFT filter
    Mlt::Profile profile;
    m_filter = new Mlt::Filter(profile, "fft");
    m_filter->set("window_size", WINDOW_SIZE);

    // Add the audio signal widget
    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(4, 4, 4, 4);
    m_audioMeter = new AudioMeterWidget(this);
    m_audioMeter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVector<int> dbscale;
    dbscale << -50 << -40 << -35 << -30 << -25 << -20 << -15 << -10 << -5 << 0;
    m_audioMeter->setDbLabels(dbscale);
    QStringList freqLabels;
    for (int i = FIRST_AUDIBLE_BAND_INDEX; i <= LAST_AUDIBLE_BAND_INDEX; i++) {
        freqLabels << BAND_TAB[i].label;
    }
    m_audioMeter->setChannelLabels(freqLabels);
    m_audioMeter->setChannelLabelUnits("Hz");
    vlayout->addWidget(m_audioMeter);

    // Config the size.
    m_audioMeter->setOrientation(Qt::Vertical);
    m_audioMeter->setMinimumSize(200, 80);
    m_audioMeter->setMaximumSize(600, 600);
    setMinimumSize(204, 84);
    setMaximumSize(604, 604);

    LOG_DEBUG() << "end";
}

AudioSpectrumScopeWidget::~AudioSpectrumScopeWidget()
{
    delete m_filter;
}

void AudioSpectrumScopeWidget::processSpectrum()
{
    QVector<double> bands(AUDIBLE_BAND_COUNT);
    float *bins = (float *) m_filter->get_data("bins");
    int bin_count = m_filter->get_int("bin_count");
    double bin_width = m_filter->get_double("bin_width");

    int band = 0;
    bool firstBandFound = false;
    for (int bin = 0; bin < bin_count; bin++) {
        // Loop through all the FFT bins and align bin frequencies with
        // band frequencies.
        double F = bin_width * (double) bin;

        if (!firstBandFound) {
            // Skip bins that come before the first band.
            if (BAND_TAB[band + FIRST_AUDIBLE_BAND_INDEX].low > F) {
                continue;
            } else {
                firstBandFound = true;
                bands[band] = bins[bin];
            }
        } else if (BAND_TAB[band + FIRST_AUDIBLE_BAND_INDEX].high < F) {
            // This bin is outside of this band - move to the next band.
            band++;
            if ((band + FIRST_AUDIBLE_BAND_INDEX) > LAST_AUDIBLE_BAND_INDEX) {
                // Skip bins that come after the last band.
                break;
            }
            bands[band] = bins[bin];
        } else if (bands[band] < bins[bin]) {
            // Pick the highest bin level within this band to represent the
            // whole band.
            bands[band] = bins[bin];
        }
    }

    // At this point, bands contains the magnitude of the signal for each
    // band. Convert to dB.
    for (band = 0; band < bands.size(); band++) {
        double mag = bands[band];
        double dB = mag > 0.0 ? 20 * log10(mag) : -1000.0;
        bands[band] = dB;
    }

    // Update the audio signal widget
    QMetaObject::invokeMethod(m_audioMeter,
                              "showAudio",
                              Qt::QueuedConnection,
                              Q_ARG(const QVector<double> &, bands));
}

void AudioSpectrumScopeWidget::refreshScope(const QSize & /*size*/, bool /*full*/)
{
    bool refresh = false;
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
            mFrame.get_audio(format, frequency, channels, samples);
            refresh = true;
        }
    }

    if (refresh) {
        processSpectrum();
    }
}

QString AudioSpectrumScopeWidget::getTitle()
{
    return tr("Audio Spectrum");
}
