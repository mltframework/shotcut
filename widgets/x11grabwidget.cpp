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

#include "x11grabwidget.h"
#include "ui_x11grabwidget.h"
#include "pulseaudiowidget.h"
#include "jackproducerwidget.h"
#include "alsawidget.h"
#include <Mlt.h>
#include <QtGui>

X11grabWidget::X11grabWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::X11grabWidget),
    m_audioWidget(0)
{
    ui->setupUi(this);
}

X11grabWidget::~X11grabWidget()
{
    delete ui;
}

void X11grabWidget::on_positionComboBox_activated(int index)
{
    ui->xSpinBox->setEnabled(index == 1);
    ui->ySpinBox->setEnabled(index == 1);
}

void X11grabWidget::on_audioComboBox_activated(int index)
{
    if (m_audioWidget)
        delete m_audioWidget;
    m_audioWidget = 0;
    if (index == 1) {
        m_audioWidget = new PulseAudioWidget(this);
        ui->gridLayout->addWidget(m_audioWidget, ui->gridLayout->rowCount() -1,
                                  0, 1, ui->gridLayout->columnCount());
    }
    else if (index == 2) {
        m_audioWidget = new JackProducerWidget(this);
        ui->gridLayout->addWidget(m_audioWidget, ui->gridLayout->rowCount() -1,
                                  0, 1, ui->gridLayout->columnCount());
    }
    else if (index == 3) {
        m_audioWidget = new AlsaWidget(this);
        ui->gridLayout->addWidget(m_audioWidget, ui->gridLayout->rowCount() -1,
                                  0, 1, ui->gridLayout->columnCount());
    }
}

QString X11grabWidget::URL(Mlt::Profile& profile) const
{
    QString s = QString("x11grab:%1+%2,%3?width=%4&height=%5&framerate=%6&show_region=%7&draw_mouse=%8&follow_mouse=%9")
            .arg(ui->lineEdit->text())
            .arg(ui->xSpinBox->value())
            .arg(ui->ySpinBox->value())
            .arg(ui->widthSpinBox->value())
            .arg(ui->heightSpinBox->value())
            .arg(profile.fps())
            .arg(ui->showRegionCheckBox->isChecked()? 1: 0)
            .arg(ui->drawMouseCheckBox->isChecked()? 1 : 0)
            .arg(ui->positionComboBox->currentIndex() - 1);
    return s;
}

Mlt::Producer* X11grabWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, URL(profile).toAscii().constData());
    if (!p->is_valid()) {
        delete p;
        p = new Mlt::Producer(profile, "color:");
        p->set("resource", QString("x11grab:%1")
               .arg(ui->lineEdit->text()).toAscii().constData());
        p->set("error", 1);
    }
    else if (m_audioWidget) {
        Mlt::Producer* audio = dynamic_cast<AbstractProducerWidget*>(m_audioWidget)->producer(profile);
        Mlt::Tractor* tractor = new Mlt::Tractor;
        tractor->set_track(*p, 0);
        delete p;
        tractor->set_track(*audio, 1);
        delete audio;
        Mlt::Transition* tran = new Mlt::Transition(profile, "mix");
        tractor->plant_transition(tran);
        delete tran;
        p = new Mlt::Producer(tractor->get_producer());
        delete tractor;
        p->set("resource", QString("x11grab:%1")
               .arg(ui->lineEdit->text()).toAscii().constData());
    }
    p->set("display", ui->lineEdit->text().toAscii().constData());
    p->set("xpos", ui->xSpinBox->value());
    p->set("ypos", ui->ySpinBox->value());
    p->set("width", ui->widthSpinBox->value());
    p->set("height", ui->heightSpinBox->value());
    p->set("show_region", ui->showRegionCheckBox->isChecked()? 1: 0);
    p->set("draw_mouse", ui->drawMouseCheckBox->isChecked()? 1: 0);
    p->set("follow_mouse", ui->positionComboBox->currentIndex() - 1);
    p->set("audio_ix", ui->audioComboBox->currentIndex());
    return p;
}

Mlt::Properties* X11grabWidget::getPreset() const
{
    Mlt::Properties* p = new Mlt::Properties;
    p->set("display", ui->lineEdit->text().toAscii().constData());
    p->set("xpos", ui->xSpinBox->value());
    p->set("ypos", ui->ySpinBox->value());
    p->set("width", ui->widthSpinBox->value());
    p->set("height", ui->heightSpinBox->value());
    p->set("show_region", ui->showRegionCheckBox->isChecked()? 1: 0);
    p->set("draw_mouse", ui->drawMouseCheckBox->isChecked()? 1: 0);
    p->set("follow_mouse", ui->positionComboBox->currentIndex() - 1);
    p->set("audio_ix", ui->audioComboBox->currentIndex());
    return p;
}

void X11grabWidget::loadPreset(Mlt::Properties& p)
{
    ui->lineEdit->setText(p.get("display"));
    ui->xSpinBox->setValue(p.get_int("xpos"));
    ui->ySpinBox->setValue(p.get_int("ypos"));
    ui->widthSpinBox->setValue(p.get_int("width"));
    ui->heightSpinBox->setValue(p.get_int("height"));
    ui->showRegionCheckBox->setChecked(p.get_int("show_region"));
    ui->drawMouseCheckBox->setChecked(p.get_int("draw_mouse"));
    ui->positionComboBox->setCurrentIndex(p.get_int("follow_mouse") + 1);
    ui->audioComboBox->setCurrentIndex(p.get_int("audio_ix"));
}
