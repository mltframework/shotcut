/*
 * Copyright (c) 2012 Meltytech, LLC
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

#include "plasmawidget.h"
#include "ui_plasmawidget.h"

PlasmaWidget::PlasmaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlasmaWidget)
{
    ui->setupUi(this);
    ui->preset->saveDefaultPreset(*getPreset());
    ui->preset->loadPresets();
}

PlasmaWidget::~PlasmaWidget()
{
    delete ui;
}

void PlasmaWidget::on_speed1Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set("1_speed", value/100.0);
        emit producerChanged();
    }
    ui->speed1Spinner->setValue(value/100.0);
}

void PlasmaWidget::on_speed1Spinner_valueChanged(double value)
{
    ui->speed1Dial->setValue(value * 100);
}

void PlasmaWidget::on_speed2Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set("2_speed", value/100.0);
        emit producerChanged();
    }
    ui->speed2Spinner->setValue(value/100.0);
}

void PlasmaWidget::on_speed2Spinner_valueChanged(double value)
{
    ui->speed2Dial->setValue(value * 100);
}

void PlasmaWidget::on_speed3Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set("3_speed", value/100.0);
        emit producerChanged();
    }
    ui->speed3Spinner->setValue(value/100.0);
}

void PlasmaWidget::on_speed3Spinner_valueChanged(double value)
{
    ui->speed3Dial->setValue(value * 100);
}

void PlasmaWidget::on_speed4Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set("4_speed", value/100.0);
        emit producerChanged();
    }
    ui->speed4Spinner->setValue(value/100.0);
}

void PlasmaWidget::on_speed4Spinner_valueChanged(double value)
{
    ui->speed4Dial->setValue(value * 100);
}

void PlasmaWidget::on_move1Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set("1_move", value/100.0);
        emit producerChanged();
    }
    ui->move1Spinner->setValue(value/100.0);
}

void PlasmaWidget::on_move1Spinner_valueChanged(double value)
{
    ui->move1Dial->setValue(value * 100);
}

void PlasmaWidget::on_move2Dial_valueChanged(int value)
{
    if (m_producer) {
        m_producer->set("2_move", value/100.0);
        emit producerChanged();
    }
    ui->move2Spinner->setValue(value/100.0);
}

void PlasmaWidget::on_move2Spinner_valueChanged(double value)
{
    ui->move2Dial->setValue(value * 100);
}

Mlt::Producer* PlasmaWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, "frei0r.plasma");
    p->set("1_speed", ui->speed1Spinner->text().toAscii().constData());
    p->set("2_speed", ui->speed2Spinner->text().toAscii().constData());
    p->set("3_speed", ui->speed3Spinner->text().toAscii().constData());
    p->set("4_speed", ui->speed4Spinner->text().toAscii().constData());
    p->set("1_move", ui->move1Spinner->text().toAscii().constData());
    p->set("2_move", ui->move2Spinner->text().toAscii().constData());
    return p;
}

Mlt::Properties* PlasmaWidget::getPreset() const
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set("1_speed", ui->speed1Spinner->text().toAscii().constData());
    p->set("2_speed", ui->speed2Spinner->text().toAscii().constData());
    p->set("3_speed", ui->speed3Spinner->text().toAscii().constData());
    p->set("4_speed", ui->speed4Spinner->text().toAscii().constData());
    p->set("1_move", ui->move1Spinner->text().toAscii().constData());
    p->set("2_move", ui->move2Spinner->text().toAscii().constData());
    return p;
}

void PlasmaWidget::loadPreset(Mlt::Properties& p)
{
    ui->speed1Spinner->setValue(p.get_double("1_speed"));
    ui->speed2Spinner->setValue(p.get_double("2_speed"));
    ui->speed3Spinner->setValue(p.get_double("3_speed"));
    ui->speed4Spinner->setValue(p.get_double("4_speed"));
    ui->move1Spinner->setValue(p.get_double("1_move"));
    ui->move2Spinner->setValue(p.get_double("2_move"));
}

void PlasmaWidget::on_preset_selected(void* p)
{
    Mlt::Properties* properties = (Mlt::Properties*) p;
    loadPreset(*properties);
    delete properties;
}

void PlasmaWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}
