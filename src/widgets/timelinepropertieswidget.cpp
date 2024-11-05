/*
 * Copyright (c) 2015-2022 Meltytech, LLC
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

#include "timelinepropertieswidget.h"
#include "ui_timelinepropertieswidget.h"
#include "mltcontroller.h"
#include "util.h"

TimelinePropertiesWidget::TimelinePropertiesWidget(Mlt::Service &service, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TimelinePropertiesWidget),
    m_service(service)
{
    ui->setupUi(this);
    connect(ui->editButton, &QAbstractButton::clicked, this, &TimelinePropertiesWidget::editProfile);
    Util::setColorsToHighlight(ui->nameLabel);
    if (m_service.is_valid()) {
        Mlt::Profile &profile = MLT.profile();
        ui->resolutionLabel->setText(QStringLiteral("%1 x %2").arg(profile.width()).arg(profile.height()));
        ui->aspectRatioLabel->setText(QStringLiteral("%1 : %2").arg(profile.display_aspect_num()).arg(
                                          profile.display_aspect_den()));
        ui->frameRateLabel->setText(tr("%L1 fps").arg(profile.fps(), 0, 'f', 6));
        if (profile.progressive())
            ui->scanModeLabel->setText(tr("Progressive"));
        else
            ui->scanModeLabel->setText(tr("Interlaced"));
        if (profile.colorspace() == 601)
            ui->colorspaceLabel->setText("ITU-R BT.601");
        else if (profile.colorspace() == 709)
            ui->colorspaceLabel->setText("ITU-R BT.709");
        else
            ui->colorspaceLabel->setText("");
    }
}

TimelinePropertiesWidget::~TimelinePropertiesWidget()
{
    delete ui;
}

