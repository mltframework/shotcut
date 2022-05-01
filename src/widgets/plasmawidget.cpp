/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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

#include "shotcut_mlt_properties.h"
#include "plasmawidget.h"
#include "ui_plasmawidget.h"
#include "util.h"

static const char *kParamSpeed1 = "0";
static const char *kParamSpeed2 = "1";
static const char *kParamSpeed3 = "2";
static const char *kParamSpeed4 = "3";
static const char *kParamMove1  = "4";
static const char *kParamMove2  = "5";

PlasmaWidget::PlasmaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlasmaWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->nameLabel);
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
}

PlasmaWidget::~PlasmaWidget()
{
    delete ui;
}

void PlasmaWidget::on_speed1Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamSpeed1, value / 100.0);
        emit producerChanged(producer());
    }
    ui->speed1Spinner->setValue(value / 100.0);
}

void PlasmaWidget::on_speed1Spinner_valueChanged(double value)
{
    ui->speed1Dial->setValue(value * 100);
}

void PlasmaWidget::on_speed2Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamSpeed2, value / 100.0);
        emit producerChanged(producer());
    }
    ui->speed2Spinner->setValue(value / 100.0);
}

void PlasmaWidget::on_speed2Spinner_valueChanged(double value)
{
    ui->speed2Dial->setValue(value * 100);
}

void PlasmaWidget::on_speed3Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamSpeed3, value / 100.0);
        emit producerChanged(producer());
    }
    ui->speed3Spinner->setValue(value / 100.0);
}

void PlasmaWidget::on_speed3Spinner_valueChanged(double value)
{
    ui->speed3Dial->setValue(value * 100);
}

void PlasmaWidget::on_speed4Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamSpeed4, value / 100.0);
        emit producerChanged(producer());
    }
    ui->speed4Spinner->setValue(value / 100.0);
}

void PlasmaWidget::on_speed4Spinner_valueChanged(double value)
{
    ui->speed4Dial->setValue(value * 100);
}

void PlasmaWidget::on_move1Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamMove1, value / 100.0);
        emit producerChanged(producer());
    }
    ui->move1Spinner->setValue(value / 100.0);
}

void PlasmaWidget::on_move1Spinner_valueChanged(double value)
{
    ui->move1Dial->setValue(value * 100);
}

void PlasmaWidget::on_move2Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set(kParamMove2, value / 100.0);
        emit producerChanged(producer());
    }
    ui->move2Spinner->setValue(value / 100.0);
}

void PlasmaWidget::on_move2Spinner_valueChanged(double value)
{
    ui->move2Dial->setValue(value * 100);
}

Mlt::Producer *PlasmaWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "frei0r.plasma");
    p->set(kParamSpeed1, ui->speed1Spinner->text().toLatin1().constData());
    p->set(kParamSpeed2, ui->speed2Spinner->text().toLatin1().constData());
    p->set(kParamSpeed3, ui->speed3Spinner->text().toLatin1().constData());
    p->set(kParamSpeed4, ui->speed4Spinner->text().toLatin1().constData());
    p->set(kParamMove1, ui->move1Spinner->text().toLatin1().constData());
    p->set(kParamMove2, ui->move2Spinner->text().toLatin1().constData());
    p->set(kShotcutCaptionProperty, ui->nameLabel->text().toUtf8().constData());
    p->set(kShotcutDetailProperty, ui->nameLabel->text().toUtf8().constData());
    return p;
}

Mlt::Properties PlasmaWidget::getPreset() const
{
    Mlt::Properties p;
    p.set(kParamSpeed1, ui->speed1Spinner->text().toLatin1().constData());
    p.set(kParamSpeed2, ui->speed2Spinner->text().toLatin1().constData());
    p.set(kParamSpeed3, ui->speed3Spinner->text().toLatin1().constData());
    p.set(kParamSpeed4, ui->speed4Spinner->text().toLatin1().constData());
    p.set(kParamMove1, ui->move1Spinner->text().toLatin1().constData());
    p.set(kParamMove2, ui->move2Spinner->text().toLatin1().constData());
    return p;
}

void PlasmaWidget::loadPreset(Mlt::Properties &p)
{
    ui->speed1Spinner->setValue(p.get_double(kParamSpeed1));
    ui->speed2Spinner->setValue(p.get_double(kParamSpeed2));
    ui->speed3Spinner->setValue(p.get_double(kParamSpeed3));
    ui->speed4Spinner->setValue(p.get_double(kParamSpeed4));
    ui->move1Spinner->setValue(p.get_double(kParamMove1));
    ui->move2Spinner->setValue(p.get_double(kParamMove2));
}

void PlasmaWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void PlasmaWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}
