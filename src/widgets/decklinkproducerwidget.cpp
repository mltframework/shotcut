/*
 * Copyright (c) 2012-2018 Meltytech, LLC
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

#include "decklinkproducerwidget.h"
#include "ui_decklinkproducerwidget.h"

#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

DecklinkProducerWidget::DecklinkProducerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DecklinkProducerWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label);
    ui->profileCombo->addItem(tr("Detect Automatically"), "auto");
    ui->profileCombo->addItem("HD 720p 50 fps", "atsc_720p_50");
    ui->profileCombo->addItem("HD 720p 59.94 fps", "atsc_720p_5994");
    ui->profileCombo->addItem("HD 720p 60 fps", "atsc_720p_60");
    ui->profileCombo->addItem("HD 1080i 25 fps", "atsc_1080i_50");
    ui->profileCombo->addItem("HD 1080i 29.97 fps", "atsc_1080i_5994");
    ui->profileCombo->addItem("HD 1080p 23.98 fps", "atsc_1080p_2398");
    ui->profileCombo->addItem("HD 1080p 24 fps", "atsc_1080p_24");
    ui->profileCombo->addItem("HD 1080p 25 fps", "atsc_1080p_25");
    ui->profileCombo->addItem("HD 1080p 29.97 fps", "atsc_1080p_2997");
    ui->profileCombo->addItem("HD 1080p 30 fps", "atsc_1080p_30");
    ui->profileCombo->addItem("SD NTSC", "dv_ntsc");
    ui->profileCombo->addItem("SD PAL", "dv_pal");
    ui->profileCombo->addItem("UHD 2160p 23.98 fps", "uhd_2160p_2398");
    ui->profileCombo->addItem("UHD 2160p 24 fps", "uhd_2160p_24");
    ui->profileCombo->addItem("UHD 2160p 25 fps", "uhd_2160p_25");
    ui->profileCombo->addItem("UHD 2160p 29.97 fps", "uhd_2160p_2997");
    ui->profileCombo->addItem("UHD 2160p 30 fps", "uhd_2160p_30");
    ui->profileCombo->addItem("UHD 2160p 50 fps", "uhd_2160p_50");
    ui->profileCombo->addItem("UHD 2160p 59.94 fps", "uhd_2160p_5994");
    ui->profileCombo->addItem("UHD 2160p 60 fps", "uhd_2160p_60");

    Mlt::Profile profile;
    Mlt::Producer p(profile, "decklink:");
    if (p.is_valid()) {
        p.set("list_devices", 1);
        int n = p.get_int("devices");
        for (int i = 0; i < n; ++i) {
            QString device(p.get(QStringLiteral("device.%1").arg(i).toLatin1().constData()));
            if (!device.isEmpty())
                ui->deviceCombo->addItem(device);
        }
    }
}

DecklinkProducerWidget::~DecklinkProducerWidget()
{
    delete ui;
}

Mlt::Producer *DecklinkProducerWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile,
                                         QStringLiteral("consumer:decklink:%1")
                                             .arg(ui->deviceCombo->currentIndex())
                                             .toLatin1()
                                             .constData());
    if (p->is_valid()) {
        p->set("profile",
               ui->profileCombo->itemData(ui->profileCombo->currentIndex())
                   .toString()
                   .toLatin1()
                   .constData());
        p->set(kBackgroundCaptureProperty, 2);
        p->set(kShotcutCaptionProperty, tr("SDI/HDMI").toUtf8().constData());
    }
    return p;
}

Mlt::Properties DecklinkProducerWidget::getPreset() const
{
    Mlt::Properties p;
    p.set("card", ui->deviceCombo->currentIndex());
    p.set("profile", ui->profileCombo->currentIndex());
    return p;
}

void DecklinkProducerWidget::loadPreset(Mlt::Properties &p)
{
    ui->deviceCombo->setCurrentIndex(p.get_int("card"));
    for (int i = 0; i < ui->profileCombo->count(); ++i) {
        if (ui->profileCombo->itemData(i).toString() == p.get("profile")) {
            ui->profileCombo->setCurrentIndex(i);
            break;
        }
    }
}

void DecklinkProducerWidget::on_deviceCombo_activated(int /*index*/)
{
    if (m_producer) {
        MLT.close();
        AbstractProducerWidget::setProducer(0);
        emit producerChanged(0);
        QCoreApplication::processEvents();

        Mlt::Producer *p = newProducer(MLT.profile());
        AbstractProducerWidget::setProducer(p);
        MLT.setProducer(p);
        MLT.play();
        emit producerChanged(p);
    }
}

void DecklinkProducerWidget::on_profileCombo_activated(int index)
{
    on_deviceCombo_activated(index);
}
