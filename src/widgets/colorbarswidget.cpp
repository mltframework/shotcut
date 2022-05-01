/*
 * Copyright (c) 2012-2019 Meltytech, LLC
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

#include "shotcut_mlt_properties.h"
#include "colorbarswidget.h"
#include "ui_colorbarswidget.h"
#include "util.h"
#include "mltcontroller.h"
#include <MltProfile.h>

static const char *kParamType = "0";
static const char *kParamAspect = "1";

enum {
    ASPECT_SQUARE = 0,
    ASPECT_PAL,
    ASPECT_PAL_WIDE,
    ASPECT_NTSC,
    ASPECT_NTSC_WIDE,
    ASPECT_HDV
};

ColorBarsWidget::ColorBarsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ColorBarsWidget)
{
    ui->setupUi(this);
    ui->comboBox->setCurrentIndex(4);
    Util::setColorsToHighlight(ui->label_2);
}

ColorBarsWidget::~ColorBarsWidget()
{
    delete ui;
}

static double map_value_backward(double v, double min, double max)
{
    return (v - min) / (max - min);
}

Mlt::Producer *ColorBarsWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "frei0r.test_pat_B");
    p->set(kParamType, ui->comboBox->currentIndex());
    if (profile.sample_aspect_num() == 16 && profile.sample_aspect_den() == 15)
        p->set(kParamAspect, map_value_backward(ASPECT_PAL, 0, 6.9999));
    else if (profile.sample_aspect_num() == 64 && profile.sample_aspect_den() == 45)
        p->set(kParamAspect, map_value_backward(ASPECT_PAL_WIDE, 0, 6.9999));
    else if (profile.sample_aspect_num() == 8 && profile.sample_aspect_den() == 9)
        p->set(kParamAspect, map_value_backward(ASPECT_NTSC, 0, 6.9999));
    else if (profile.sample_aspect_num() == 32 && profile.sample_aspect_den() == 27)
        p->set(kParamAspect, map_value_backward(ASPECT_NTSC_WIDE, 0, 6.9999));
    else if (profile.sample_aspect_num() == 4 && profile.sample_aspect_den() == 3)
        p->set(kParamAspect, map_value_backward(ASPECT_HDV, 0, 6.9999));
    MLT.setDurationFromDefault(p);
    p->set(kShotcutCaptionProperty, ui->comboBox->currentText().toUtf8().constData());
    p->set(kShotcutDetailProperty, ui->comboBox->currentText().toUtf8().constData());
    return p;
}

Mlt::Properties ColorBarsWidget::getPreset() const
{
    Mlt::Properties p;
    p.set(kParamType, ui->comboBox->currentIndex());
    return p;
}

void ColorBarsWidget::loadPreset(Mlt::Properties &p)
{
    ui->comboBox->setCurrentIndex(p.get_int(kParamType));
}

void ColorBarsWidget::on_comboBox_activated(int index)
{
    if (m_producer) {
        m_producer->set(kParamType, index);
        m_producer->set(kShotcutCaptionProperty, ui->comboBox->currentText().toUtf8().constData());
        m_producer->set(kShotcutDetailProperty, ui->comboBox->currentText().toUtf8().constData());
        emit producerChanged(producer());
    }
}
