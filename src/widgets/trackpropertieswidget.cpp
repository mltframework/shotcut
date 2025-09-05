/*
 * Copyright (c) 2015-2019 Meltytech, LLC
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

#include "trackpropertieswidget.h"
#include "ui_trackpropertieswidget.h"

#include "commands/timelinecommands.h"
#include "mainwindow.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <Mlt.h>
#include <QScopedPointer>

static const char *BLEND_PROPERTY_CAIROBLEND = "1";
static const char *BLEND_PROPERTY_QTBLEND = "compositing";

TrackPropertiesWidget::TrackPropertiesWidget(Mlt::Producer &track, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrackPropertiesWidget)
    , m_track(track)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->nameLabel);
    ui->nameLabel->setText(tr("Track: %1").arg(track.get(kTrackNameProperty)));
    ui->blendModeLabel->hide();
    ui->blendModeCombo->hide();

    QScopedPointer<Mlt::Transition> transition(getTransition("qtblend"));
    if (transition && transition->is_valid()) {
        ui->blendModeCombo->blockSignals(true);
        ui->blendModeCombo->addItem(tr("Source Over"), "0");
        ui->blendModeCombo->addItem(tr("Destination Over"), "1");
        ui->blendModeCombo->addItem(tr("Clear"), "2");
        ui->blendModeCombo->addItem(tr("Source"), "3");
        ui->blendModeCombo->addItem(tr("Destination"), "4");
        ui->blendModeCombo->addItem(tr("Source In"), "5");
        ui->blendModeCombo->addItem(tr("Destination In"), "6");
        ui->blendModeCombo->addItem(tr("Source Out"), "7");
        ui->blendModeCombo->addItem(tr("Destination Out"), "8");
        ui->blendModeCombo->addItem(tr("Source Atop"), "9");
        ui->blendModeCombo->addItem(tr("Destination Atop"), "10");
        ui->blendModeCombo->addItem(tr("XOR"), "11");
        ui->blendModeCombo->addItem(tr("Plus"), "12");
        ui->blendModeCombo->addItem(tr("Multiply"), "13");
        ui->blendModeCombo->addItem(tr("Screen"), "14");
        ui->blendModeCombo->addItem(tr("Overlay"), "15");
        ui->blendModeCombo->addItem(tr("Darken"), "16");
        ui->blendModeCombo->addItem(tr("Lighten"), "17");
        ui->blendModeCombo->addItem(tr("Color Dodge"), "18");
        ui->blendModeCombo->addItem(tr("Color Burn"), "19");
        ui->blendModeCombo->addItem(tr("Hard Light"), "20");
        ui->blendModeCombo->addItem(tr("Soft Light"), "21");
        ui->blendModeCombo->addItem(tr("Difference"), "22");
        ui->blendModeCombo->addItem(tr("Exclusion"), "23");
        /*
        ui->blendModeCombo->addItem(tr("Source OR Destination"), "24");
        ui->blendModeCombo->addItem(tr("Source AND Destination"), "25");
        ui->blendModeCombo->addItem(tr("Source XOR Destination"), "26");
        ui->blendModeCombo->addItem(tr("NOT Source AND NOT Destination"), "27");
        ui->blendModeCombo->addItem(tr("NOT Source OR NOT Destination"), "28");
        ui->blendModeCombo->addItem(tr("NOT Source XOR Destination"), "29");
        ui->blendModeCombo->addItem(tr("NOT Source"), "30");
        ui->blendModeCombo->addItem(tr("NOT Source AND Destination"), "31");
        ui->blendModeCombo->addItem(tr("Source AND NOT Destination"), "32");
        ui->blendModeCombo->addItem(tr("NOT Source OR Destination"), "33");
        ui->blendModeCombo->addItem(tr("Source OR NOT Destination"), "34");
        ui->blendModeCombo->addItem(tr("NOT Destination"), "37");
         */
        ui->blendModeCombo->blockSignals(false);
        ui->blendModeLabel->show();
        ui->blendModeCombo->show();

        QString blendMode = transition->get(BLEND_PROPERTY_QTBLEND);
        if (transition->get_int("disable"))
            blendMode = QString();
        else if (blendMode.isEmpty()) // A newly added track does not set its mode property.
            blendMode = "0";
        onModeChanged(blendMode);
    } else {
        transition.reset(getTransition("frei0r.cairoblend"));
        if (transition && transition->is_valid()) {
            ui->blendModeCombo->blockSignals(true);
            ui->blendModeCombo->addItem(tr("None"), "");
            ui->blendModeCombo->addItem(tr("Over"), "normal");
            ui->blendModeCombo->addItem(tr("Add"), "add");
            ui->blendModeCombo->addItem(tr("Saturate"), "saturate");
            ui->blendModeCombo->addItem(tr("Multiply"), "multiply");
            ui->blendModeCombo->addItem(tr("Screen"), "screen");
            ui->blendModeCombo->addItem(tr("Overlay"), "overlay");
            ui->blendModeCombo->addItem(tr("Darken"), "darken");
            ui->blendModeCombo->addItem(tr("Dodge"), "colordodge");
            ui->blendModeCombo->addItem(tr("Burn"), "colorburn");
            ui->blendModeCombo->addItem(tr("Hard Light"), "hardlight");
            ui->blendModeCombo->addItem(tr("Soft Light"), "softlight");
            ui->blendModeCombo->addItem(tr("Difference"), "difference");
            ui->blendModeCombo->addItem(tr("Exclusion"), "exclusion");
            ui->blendModeCombo->addItem(tr("HSL Hue"), "hslhue");
            ui->blendModeCombo->addItem(tr("HSL Saturation"), "hslsaturatation");
            ui->blendModeCombo->addItem(tr("HSL Color"), "hslcolor");
            ui->blendModeCombo->addItem(tr("HSL Luminosity"), "hslluminocity");
            ui->blendModeCombo->blockSignals(false);
            ui->blendModeLabel->show();
            ui->blendModeCombo->show();

            QString blendMode = transition->get(BLEND_PROPERTY_CAIROBLEND);
            if (transition->get_int("disable"))
                blendMode = QString();
            else if (blendMode.isEmpty()) // A newly added track does not set its mode property.
                blendMode = "normal";
            onModeChanged(blendMode);
        } else {
            transition.reset(getTransition("movit.overlay"));
            if (transition && transition->is_valid()) {
                ui->blendModeCombo->blockSignals(true);
                ui->blendModeCombo->addItem(tr("None"), "");
                ui->blendModeCombo->addItem(tr("Over"), "over");
                ui->blendModeCombo->blockSignals(false);
                ui->blendModeLabel->show();
                ui->blendModeCombo->show();
                QString blendMode = transition->get_int("disable") ? QString() : "over";
                onModeChanged(blendMode);
            }
        }
    }
}

TrackPropertiesWidget::~TrackPropertiesWidget()
{
    delete ui;
}

Mlt::Transition *TrackPropertiesWidget::getTransition(const QString &name)
{
    // track.consumer() is the multitrack
    QScopedPointer<Mlt::Service> service(m_track.consumer());
    if (service && service->is_valid()) {
        Mlt::Multitrack multi(*service);
        int trackIndex;

        // Get the track index by iterating until multitrack.track() == track.get_producer().
        for (trackIndex = 0; trackIndex < multi.count(); ++trackIndex) {
            QScopedPointer<Mlt::Producer> producer(multi.track(trackIndex));
            if (producer->get_producer() == m_track.get_producer())
                break;
        }

        // Iterate the consumers until found transition by mlt_service and track_b index.
        while (service && service->is_valid() && mlt_service_tractor_type != service->type()) {
            if (service->type() == mlt_service_transition_type) {
                Mlt::Transition t((mlt_transition) service->get_service());
                if (name == t.get("mlt_service") && t.get_b_track() == trackIndex)
                    return new Mlt::Transition(t);
            }
            service.reset(service->consumer());
        };
    }
    return 0;
}

void TrackPropertiesWidget::on_blendModeCombo_currentIndexChanged(int index)
{
    if (index >= 0) {
        QScopedPointer<Mlt::Transition> transition(getTransition("frei0r.cairoblend"));
        if (!transition)
            transition.reset(getTransition("movit.overlay"));
        if (transition && transition->is_valid()) {
            Timeline::ChangeBlendModeCommand *command
                = new Timeline::ChangeBlendModeCommand(*transition,
                                                       BLEND_PROPERTY_CAIROBLEND,
                                                       ui->blendModeCombo->itemData(index)
                                                           .toString());
            connect(command, SIGNAL(modeChanged(QString &)), SLOT(onModeChanged(QString &)));
            MAIN.undoStack()->push(command);
        } else {
            transition.reset(getTransition("qtblend"));
            if (transition && transition->is_valid()) {
                Timeline::ChangeBlendModeCommand *command
                    = new Timeline::ChangeBlendModeCommand(*transition,
                                                           BLEND_PROPERTY_QTBLEND,
                                                           ui->blendModeCombo->itemData(index)
                                                               .toString());
                connect(command, SIGNAL(modeChanged(QString &)), SLOT(onModeChanged(QString &)));
                MAIN.undoStack()->push(command);
            }
        }
    }
}

void TrackPropertiesWidget::onModeChanged(QString &mode)
{
    for (int i = 0; i < ui->blendModeCombo->count(); ++i) {
        if (ui->blendModeCombo->itemData(i).toString() == mode) {
            ui->blendModeCombo->blockSignals(true);
            ui->blendModeCombo->setCurrentIndex(i);
            ui->blendModeCombo->blockSignals(false);
            break;
        }
    }
}
