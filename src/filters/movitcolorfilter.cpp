/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "movitcolorfilter.h"
#include "ui_movitcolorfilter.h"
#include "mltcontroller.h"

static const double LIFT_FACTOR = 1.0;
static const double GAMMA_FACTOR = 2.0;
static const double GAIN_FACTOR = 4.0;

MovitColorFilter::MovitColorFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MovitColorFilter),
    m_filter(filter)
{
    ui->setupUi(this);
    Mlt::Filter f(MLT.profile(), "movit.lift_gamma_gain");
    m_liftDefault = QColor::fromRgbF(f.get_double("lift_r"),
                                     f.get_double("lift_g"),
                                     f.get_double("lift_b"));
    m_gammaDefault = QColor::fromRgbF(f.get_double("gamma_r") / GAMMA_FACTOR,
                                      f.get_double("gamma_g") / GAMMA_FACTOR,
                                      f.get_double("gamma_b") / GAMMA_FACTOR);
    m_gainDefault = QColor::fromRgbF(f.get_double("gain_r") / GAIN_FACTOR,
                                     f.get_double("gain_g") / GAIN_FACTOR,
                                     f.get_double("gain_b") / GAIN_FACTOR);
    if (setDefaults) {
        ui->liftWheel->setColor(m_liftDefault);
        ui->gammaWheel->setColor(m_gammaDefault);
        ui->gainWheel->setColor(m_gainDefault);
        ui->preset->saveDefaultPreset(m_filter);
    } else {
        ui->liftWheel->setColor(QColor::fromRgbF(filter.get_double("lift_r"),
                                                 filter.get_double("lift_g"),
                                                 filter.get_double("lift_b")));
        ui->gammaWheel->setColor(QColor::fromRgbF(filter.get_double("gamma_r") / GAMMA_FACTOR,
                                                  filter.get_double("gamma_g") / GAMMA_FACTOR,
                                                  filter.get_double("gamma_b") / GAMMA_FACTOR));
        ui->gainWheel->setColor(QColor::fromRgbF(filter.get_double("gain_r") / GAIN_FACTOR,
                                                 filter.get_double("gain_g") / GAIN_FACTOR,
                                                 filter.get_double("gain_b") / GAIN_FACTOR));
    }
    ui->preset->loadPresets();
}

MovitColorFilter::~MovitColorFilter()
{
    delete ui;
}

void MovitColorFilter::on_liftWheel_colorChanged(const QColor &color)
{
    m_filter.set("lift_r", color.redF());
    m_filter.set("lift_g", color.greenF());
    m_filter.set("lift_b", color.blueF());
    MLT.refreshConsumer();
}

void MovitColorFilter::on_gammaWheel_colorChanged(const QColor &color)
{
    m_filter.set("gamma_r", color.redF() * 2.0);
    m_filter.set("gamma_g", color.greenF() * 2.0);
    m_filter.set("gamma_b", color.blueF() * 2.0);
    MLT.refreshConsumer();
}

void MovitColorFilter::on_gainWheel_colorChanged(const QColor &color)
{
    m_filter.set("gain_r", color.redF() * 4.0);
    m_filter.set("gain_g", color.greenF() * 4.0);
    m_filter.set("gain_b", color.blueF() * 4.0);
    MLT.refreshConsumer();
}

void MovitColorFilter::on_preset_selected(void* o)
{
    Mlt::Properties p(((Mlt::Properties*) o)->get_properties());
    if (p.get("lift_r") && p.get("lift_g") && p.get("lift_b"))
        ui->liftWheel->changeColor(QColor::fromRgbF(p.get_double("lift_r"),
                                                    p.get_double("lift_g"),
                                                    p.get_double("lift_b")));
    if (p.get("gamma_r") && p.get("gamma_g") && p.get("gamma_b"))
        ui->gammaWheel->changeColor(QColor::fromRgbF(p.get_double("gamma_r") / GAMMA_FACTOR,
                                                     p.get_double("gamma_g") / GAMMA_FACTOR,
                                                     p.get_double("gamma_b") / GAMMA_FACTOR));
    if (p.get("gain_r") && p.get("gain_g") && p.get("gain_b"))
        ui->gainWheel->changeColor(QColor::fromRgbF(p.get_double("gain_r") / GAIN_FACTOR,
                                                    p.get_double("gain_g") / GAIN_FACTOR,
                                                    p.get_double("gain_b") / GAIN_FACTOR));
}

void MovitColorFilter::on_preset_saveClicked()
{
    ui->preset->savePreset(&m_filter);
}
