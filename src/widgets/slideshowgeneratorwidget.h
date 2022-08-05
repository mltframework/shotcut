/*
 * Copyright (c) 2020-2022 Meltytech, LLC
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

#ifndef SLIDESHOWGENERATORWIDGET_H
#define SLIDESHOWGENERATORWIDGET_H

#include <MltProducer.h>

#include <QFuture>
#include <QMutex>
#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QSlider;
class QSpinBox;
namespace Mlt {
class Filter;
class Playlist;
class Transition;
}
class ProducerPreviewWidget;

class SlideshowGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    SlideshowGeneratorWidget(Mlt::Playlist *clips, QWidget *parent = 0);
    virtual ~SlideshowGeneratorWidget();

    Mlt::Playlist *getSlideshow();

private slots:
    void on_parameterChanged();

private:
    struct SlideshowConfig {
        double clipDuration;
        int aspectConversion;
        int zoomPercent;
        double transitionDuration;
        int transitionStyle;
        int transitionSoftness;
    };

    void attachAffineFilter(SlideshowConfig &config, Mlt::Producer *producer, int endPosition);
    void attachBlurFilter(SlideshowConfig &config, Mlt::Producer *producer);
    void applyLumaTransitionProperties(Mlt::Transition *luma, SlideshowConfig &config);
    void generatePreviewSlideshow();
    Q_INVOKABLE void startPreview();

    QDoubleSpinBox *m_clipDurationSpinner;
    QComboBox *m_aspectConversionCombo;
    QSpinBox *m_zoomPercentSpinner;
    QDoubleSpinBox *m_transitionDurationSpinner;
    QComboBox *m_transitionStyleCombo;
    QSpinBox *m_softnessSpinner;
    ProducerPreviewWidget *m_preview;
    Mlt::Playlist *m_clips;

    // Mutext Protected Members
    QFuture<void> m_future;
    QMutex m_mutex;
    bool m_refreshPreview;
    SlideshowConfig m_config;
    Mlt::Producer m_previewProducer;
};

#endif // SLIDESHOWGENERATORWIDGET_H
