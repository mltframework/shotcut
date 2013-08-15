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

#include "whitebalancefilter.h"
#include "ui_whitebalancefilter.h"
#include "mltcontroller.h"
#include <QSettings>

static const char* kFrei0rNeutralParam = "0";
static const char* kFrei0rTemperatureParam = "1";

static const char* kMovitNeutralParam = "neutral_color";
static const char* kMovitTemperatureParam = "color_temperature";

WhiteBalanceFilter::WhiteBalanceFilter(Mlt::Filter &filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WhiteBalanceFilter),
    m_filter(filter),
    m_defaultColor("#7f7f7f"),
    m_defaultTemperature(6500.0)
{
    m_isMovit = QString(m_filter.get("mlt_service")).startsWith("movit.");
    ui->setupUi(this);
    if (setDefaults) {
        ui->colorTemperatureSpinner->setValue(m_defaultTemperature);
        on_colorTemperatureSpinner_valueChanged(m_defaultTemperature);
        if (m_isMovit)
            m_filter.set(kMovitNeutralParam, m_defaultColor.toLatin1().constData());
        else
            m_filter.set(kFrei0rNeutralParam, m_defaultColor.toLatin1().constData());
    }
    else {
        if (m_isMovit)
            ui->colorTemperatureSpinner->setValue(m_filter.get_double(kMovitTemperatureParam));
        else
            ui->colorTemperatureSpinner->setValue(m_filter.get_double(kFrei0rTemperatureParam) * 15000.0);
    }
}

WhiteBalanceFilter::~WhiteBalanceFilter()
{
    delete ui;
}

void WhiteBalanceFilter::on_colorPicker_colorPicked(const QColor &color)
{
    if (m_isMovit)
        m_filter.set(kMovitNeutralParam, color.name().toLatin1().constData());
    else
        m_filter.set(kFrei0rNeutralParam, color.name().toLatin1().constData());
    MLT.refreshConsumer();
}

void WhiteBalanceFilter::on_colorPicker_disableCurrentFilter(bool disable)
{
    QSettings settings;
    m_filter.set("disable", disable);
    // GPU processing requires that we restart the consumer for reasons
    // internal to MLT and its integration of Movit.
    if (settings.value("player/gpu", false).toBool()) {
        double speed = MLT.producer()->get_speed();
        MLT.consumer()->stop();
        MLT.play(speed);
    }
}

void WhiteBalanceFilter::on_defaultTemperatureButton_clicked()
{
    ui->colorTemperatureSpinner->setValue(m_defaultTemperature);
}

void WhiteBalanceFilter::on_colorTemperatureSpinner_valueChanged(double arg1)
{
    if (m_isMovit)
        m_filter.set(kMovitTemperatureParam, arg1);
    else
        m_filter.set(kFrei0rTemperatureParam, arg1 / 15000.0);
    MLT.refreshConsumer();
}

void WhiteBalanceFilter::on_defaultNeutralButton_clicked()
{
    on_colorPicker_colorPicked(QColor(m_defaultColor));
}
