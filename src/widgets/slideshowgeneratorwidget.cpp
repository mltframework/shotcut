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

#include "slideshowgeneratorwidget.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "widgets/producerpreviewwidget.h"

#include <MltTransition.h>

#include <QComboBox>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

#include <math.h>

SlideshowGeneratorWidget::SlideshowGeneratorWidget(Mlt::Playlist* clips, QWidget *parent)
    : QWidget(parent)
    , m_clips(clips)
{
    QGridLayout* grid = new QGridLayout();
    setLayout(grid);

    grid->addWidget(new QLabel(tr("Clip Duration")), 0, 0, Qt::AlignRight);
    m_clipDurationSpinner = new QSpinBox();
    m_clipDurationSpinner->setMinimum(4);
    m_clipDurationSpinner->setMaximum(600);
    m_clipDurationSpinner->setValue(10);
    connect(m_clipDurationSpinner, SIGNAL(valueChanged(int)), this, SLOT(on_parameterChanged()));
    grid->addWidget(m_clipDurationSpinner, 0, 1);

    grid->addWidget(new QLabel(tr("Transition Duration")), 1, 0, Qt::AlignRight);
    m_transitionDurationSpinner = new QSpinBox();
    m_transitionDurationSpinner->setToolTip(tr("Set the duration of the transition.\nMay not be longer than half the duration of the clip.\nIf the duration is 0, no transition will be created."));
    m_transitionDurationSpinner->setMinimum(0);
    m_transitionDurationSpinner->setMaximum(10);
    m_transitionDurationSpinner->setValue(2);
    connect(m_transitionDurationSpinner, SIGNAL(valueChanged(int)), this, SLOT(on_parameterChanged()));
    grid->addWidget(m_transitionDurationSpinner, 1, 1);

    grid->addWidget(new QLabel(tr("Transition Type")), 2, 0, Qt::AlignRight);
    m_transitionStyleCombo = new QComboBox();
    m_transitionStyleCombo->addItem(tr("Random"));
    m_transitionStyleCombo->addItem(tr("Dissolve"));
    m_transitionStyleCombo->addItem(tr("Bar Horizontal"));
    m_transitionStyleCombo->addItem(tr("Bar Vertical"));
    m_transitionStyleCombo->addItem(tr("Barn Door Horizontal"));
    m_transitionStyleCombo->addItem(tr("Barn Door Vertical"));
    m_transitionStyleCombo->addItem(tr("Barn Door Diagonal SW-NE"));
    m_transitionStyleCombo->addItem(tr("Barn Door Diagonal NW-SE"));
    m_transitionStyleCombo->addItem(tr("Diagonal Top Left"));
    m_transitionStyleCombo->addItem(tr("Diagonal Top Right"));
    m_transitionStyleCombo->addItem(tr("Matrix Waterfall Horizontal"));
    m_transitionStyleCombo->addItem(tr("Matrix Waterfall Vertical"));
    m_transitionStyleCombo->addItem(tr("Matrix Snake Horizontal"));
    m_transitionStyleCombo->addItem(tr("Matrix Snake Parallel Horizontal"));
    m_transitionStyleCombo->addItem(tr("Matrix Snake Vertical"));
    m_transitionStyleCombo->addItem(tr("Matrix Snake Parallel Vertical"));
    m_transitionStyleCombo->addItem(tr("Barn V Up"));
    m_transitionStyleCombo->addItem(tr("Iris Circle"));
    m_transitionStyleCombo->addItem(tr("Double Iris"));
    m_transitionStyleCombo->addItem(tr("Iris Box"));
    m_transitionStyleCombo->addItem(tr("Box Bottom Right"));
    m_transitionStyleCombo->addItem(tr("Box Bottom Left"));
    m_transitionStyleCombo->addItem(tr("Box Right Center"));
    m_transitionStyleCombo->addItem(tr("Clock Top"));
    m_transitionStyleCombo->setToolTip(tr("Choose a transition effect"));
    m_transitionStyleCombo->setCurrentIndex(1);
    connect(m_transitionStyleCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(on_parameterChanged()));
    grid->addWidget(m_transitionStyleCombo, 2, 1);

    grid->addWidget(new QLabel(tr("Transition Softness")), 3, 0, Qt::AlignRight);
    m_softnessSpinner = new QSpinBox();
    m_softnessSpinner->setMaximum(100);
    m_softnessSpinner->setMinimum(0);
    m_softnessSpinner->setValue(20);
    m_softnessSpinner->setToolTip(tr("Change the softness of the edge of the wipe"));
    connect(m_softnessSpinner, SIGNAL(valueChanged(int)), this, SLOT(on_parameterChanged()));
    grid->addWidget(m_softnessSpinner, 3, 1);

    m_preview = new ProducerPreviewWidget();
    grid->addWidget(m_preview, 4, 0, 1, 2, Qt::AlignCenter);

    m_preview->start(getSlideshow());
}

SlideshowGeneratorWidget::~SlideshowGeneratorWidget()
{
    m_preview->stop();
}

Mlt::Playlist* SlideshowGeneratorWidget::getSlideshow()
{
    int framesPerClip = ceil((double)m_clipDurationSpinner->value() * m_clips->profile()->fps());
    Mlt::ClipInfo info;
    int count = m_clips->count();
    Mlt::Playlist* slideshow = new Mlt::Playlist(*m_clips->profile());

    for (int i = 0; i < count; i++)
    {
        Mlt::ClipInfo* c = m_clips->clip_info(i, &info);
        if (c)
        {
            slideshow->append(*c->producer, c->frame_in, c->frame_in + framesPerClip - 1);
        }
    }

    int framesPerTransition = ceil((double)m_transitionDurationSpinner->value() * m_clips->profile()->fps());
    if (framesPerTransition > (framesPerClip / 2 - 1))
    {
        framesPerTransition = (framesPerClip / 2 - 1);
    }
    if (framesPerTransition > 0)
    {
        for (int i = 0; i < count - 1; i++)
        {
            // Create playlist mix
            slideshow->mix(i, framesPerTransition);
            QScopedPointer<Mlt::Producer> producer(slideshow->get_clip(i + 1));
            if( producer.isNull() )
            {
                break;
            }
            producer->parent().set(kShotcutTransitionProperty, "lumaMix");

            // Add mix transition
            Mlt::Transition crossFade(*m_clips->profile(), "mix:-1");
            slideshow->mix_add(i + 1, &crossFade);

            // Add luma transition
            Mlt::Transition luma(*m_clips->profile(), Settings.playerGPU()? "movit.luma_mix" : "luma");
            applyLumaProperties(&luma);
            slideshow->mix_add(i + 1, &luma);

            count++;
            i++;
        }
    }

    return slideshow;
}

void SlideshowGeneratorWidget::applyLumaProperties(Mlt::Transition* luma)
{
    int index = m_transitionStyleCombo->currentIndex();

    if (index == 0) {
        // Random: pick any number other than 0
        index = rand() % 24 + 1;
    }

    if (index == 1) {
        // Dissolve
        luma->set("resource", "");
    } else {
        luma->set("resource", QString("%luma%1.pgm").arg(index - 1, 2, 10, QChar('0')).toLatin1().constData());
    }
    luma->set("softness", m_softnessSpinner->value() / 100.0);
    luma->set("progressive", 1);
    if (!Settings.playerGPU()) luma->set("alpha_over", 1);
}

void SlideshowGeneratorWidget::on_parameterChanged()
{
    if (m_transitionDurationSpinner->value() > m_clipDurationSpinner->value() / 2 )
    {
        m_transitionDurationSpinner->setValue(m_clipDurationSpinner->value() / 2);
    }
    if (m_transitionDurationSpinner->value() == 0 )
    {
        m_transitionStyleCombo->setEnabled(false);
        m_softnessSpinner->setEnabled(false);
    }
    else
    {
        m_transitionStyleCombo->setEnabled(true);
        m_softnessSpinner->setEnabled(true);
    }
    m_preview->stop();
    m_preview->start(getSlideshow());
}
