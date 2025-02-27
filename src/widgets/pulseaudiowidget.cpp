/*
 * Copyright (c) 2012-2017 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "pulseaudiowidget.h"
#include "ui_pulseaudiowidget.h"

#include "shotcut_mlt_properties.h"
#include "util.h"

PulseAudioWidget::PulseAudioWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PulseAudioWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label);
}

PulseAudioWidget::~PulseAudioWidget()
{
    delete ui;
}

Mlt::Producer *PulseAudioWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "pulse:default?name=Shotcut");
    p->set(kBackgroundCaptureProperty, 1);
    p->set(kShotcutCaptionProperty, "PulseAudio");
    return p;
}
