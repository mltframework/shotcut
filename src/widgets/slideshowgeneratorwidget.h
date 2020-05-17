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

#ifndef SLIDESHOWGENERATORWIDGET_H
#define SLIDESHOWGENERATORWIDGET_H

#include <QWidget>

class QComboBox;
class QSlider;
class QSpinBox;
namespace Mlt {
    class Filter;
    class Playlist;
    class Producer;
    class Transition;
}
class ProducerPreviewWidget;

class SlideshowGeneratorWidget : public QWidget
{
    Q_OBJECT

public:
    SlideshowGeneratorWidget(Mlt::Playlist* clips, QWidget *parent = 0);
    virtual ~SlideshowGeneratorWidget();

    Mlt::Playlist* getSlideshow();

private slots:
    void on_parameterChanged();

private:
    void applyAffineFilterProperties(Mlt::Filter* filter, Mlt::Producer* producer, int endPosition);
    void applyLumaTransitionProperties(Mlt::Transition* luma);

    QSpinBox* m_clipDurationSpinner;
    QComboBox* m_aspectConversionCombo;
    QSpinBox* m_zoomPercentSpinner;
    QSpinBox* m_transitionDurationSpinner;
    QComboBox* m_transitionStyleCombo;
    QSpinBox* m_softnessSpinner;
    ProducerPreviewWidget* m_preview;
    Mlt::Playlist* m_clips;
};

#endif // SLIDESHOWGENERATORWIDGET_H
