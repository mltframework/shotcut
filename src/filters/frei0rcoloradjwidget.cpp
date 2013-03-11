/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * GL shader based on BSD licensed code from Peter Bengtsson:
 * http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
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

#include "frei0rcoloradjwidget.h"
#include "ui_frei0rcoloradjwidget.h"
#include "mltcontroller.h"

Frei0rColoradjWidget::Frei0rColoradjWidget(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Frei0rColoradjWidget),
    m_filter(filter)
{
    ui->setupUi(this);
    m_defaultAction = 0.5;
    m_defaultLuma = true;
    m_defaultRGB = QColor::fromRgbF(0.5, 0.5, 0.5);
    if (setDefaults) {
        ui->modeComboBox->setCurrentIndex(2 * m_defaultAction);
        ui->keepLumaCheckBox->setChecked(m_defaultLuma);
        ui->wheel->setColor(m_defaultRGB);
        ui->preset->saveDefaultPreset(m_filter);
        m_filter.set("Action", m_defaultAction);
        m_filter.set("Keep luma", m_defaultLuma? 1 : 0);
        m_filter.set("R", m_defaultRGB.redF());
        m_filter.set("G", m_defaultRGB.greenF());
        m_filter.set("B", m_defaultRGB.blueF());
    } else {
        ui->modeComboBox->setCurrentIndex(2 * filter.get_double("Action"));
        ui->keepLumaCheckBox->setChecked(filter.get_int("Keep luma"));
        ui->wheel->setColor(QColor::fromRgbF(filter.get_double("R"),
                                             filter.get_double("G"),
                                             filter.get_double("B")));
    }
    ui->preset->loadPresets();
    m_filter.set("Luma formula", (MLT.profile().colorspace() == 709? 1 : 0));
}

Frei0rColoradjWidget::~Frei0rColoradjWidget()
{
    delete ui;
}

void Frei0rColoradjWidget::on_preset_selected(void* o)
{
    Mlt::Properties p(((Mlt::Properties*) o)->get_properties());
    if (p.get("Action"))
        ui->modeComboBox->setCurrentIndex(2 * p.get_double("Action"));
    if (p.get("Keep luma"))
        ui->keepLumaCheckBox->setChecked(p.get_int("Keep luma"));
    if (p.get("R") && p.get("G") && p.get("B"))
        ui->wheel->changeColor(QColor::fromRgbF(p.get_double("R"),
                                                p.get_double("G"),
                                                p.get_double("B")));
}

void Frei0rColoradjWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(&m_filter);
}

void Frei0rColoradjWidget::on_wheel_colorChanged(const QColor &color)
{
    m_filter.set("R", color.redF());
    m_filter.set("G", color.greenF());
    m_filter.set("B", color.blueF());
    MLT.refreshConsumer();
}


void Frei0rColoradjWidget::on_modeComboBox_currentIndexChanged(int index)
{
    m_filter.set("Action", double(index) / 2.0);
    MLT.refreshConsumer();
}

void Frei0rColoradjWidget::on_keepLumaCheckBox_toggled(bool checked)
{
    m_filter.set("Keep luma", (checked? 1 : 0));
    MLT.refreshConsumer();
}
