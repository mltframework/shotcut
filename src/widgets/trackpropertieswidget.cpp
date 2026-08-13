/*
 * Copyright (c) 2015-2026 Meltytech, LLC
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
#include <QTimer>

static const char *BLEND_PROPERTY_CAIROBLEND = "1";
static const char *BLEND_PROPERTY_QTBLEND = "compositing";
static const char *MIX_PROPERTY_DUCK_THRESHOLD = "duck_threshold";
static const char *MIX_PROPERTY_DUCK_ATTENUATION = "duck_attenuation";
static const char *MIX_PROPERTY_DUCK_FADE_IN = "duck_fade_in";
static const char *MIX_PROPERTY_DUCK_FADE_OUT = "duck_fade_out";
static const char *MIX_PROPERTY_DUCK_LEVEL = "duck_level";

TrackPropertiesWidget::TrackPropertiesWidget(Mlt::Producer &track,
                                             bool showBlend,
                                             bool showDucking,
                                             QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TrackPropertiesWidget)
    , m_track(track)
    , m_duckStatusTimer(new QTimer(this))
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->nameLabel);
    ui->nameLabel->setText(tr("Track: %1").arg(track.get(kTrackNameProperty)));
    ui->blendModeLabel->hide();
    ui->blendModeCombo->hide();
    setDuckingVisible(false);

    if (showDucking) {
        QScopedPointer<Mlt::Transition> mixTransition(getTransition("mix"));
        if (mixTransition && mixTransition->is_valid()) {
            onDuckThresholdChanged(mixTransition->get_double(MIX_PROPERTY_DUCK_THRESHOLD));
            onDuckAttenuationChanged(mixTransition->get_double(MIX_PROPERTY_DUCK_ATTENUATION));
            onDuckFadeInChanged(mixTransition->get_double(MIX_PROPERTY_DUCK_FADE_IN));
            onDuckFadeOutChanged(mixTransition->get_double(MIX_PROPERTY_DUCK_FADE_OUT));
            updateDuckStatus(mixTransition->get_double(MIX_PROPERTY_DUCK_LEVEL));
            setDuckingVisible(true);
        }
    }

    m_duckStatusTimer->setInterval(100);
    if (showDucking) {
        connect(m_duckStatusTimer,
                &QTimer::timeout,
                this,
                &TrackPropertiesWidget::refreshDuckStatus);
        m_duckStatusTimer->start();
    }

    if (showBlend) {
        QScopedPointer<Mlt::Transition> transition(getTransition("qtblend"));
        if (!transition)
            transition.reset(getTransition("movit.overlay"));
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
            }
        }
    }
}

TrackPropertiesWidget::~TrackPropertiesWidget()
{
    m_duckStatusTimer->stop();
    delete ui;
}

void TrackPropertiesWidget::setDuckingVisible(bool visible)
{
    ui->duckingHeadingLabel->setVisible(visible);
    ui->duckingTipLabel->setVisible(visible);
    ui->duckThresholdLabel->setVisible(visible);
    ui->duckThresholdSpinBox->setVisible(visible);
    ui->duckAttenuationLabel->setVisible(visible);
    ui->duckAttenuationSpinBox->setVisible(visible);
    ui->duckFadeInLabel->setVisible(visible);
    ui->duckFadeInSpinBox->setVisible(visible);
    ui->duckFadeOutLabel->setVisible(visible);
    ui->duckFadeOutSpinBox->setVisible(visible);
    ui->duckStatusLabel->setVisible(visible);
    ui->duckStatusValueLabel->setVisible(visible);
}

void TrackPropertiesWidget::updateDuckStatus(double value)
{
    const double clamped = qBound(0.0, value, 70.0);
    ui->duckStatusValueLabel->setValue(qRound(clamped * 10.0));
    ui->duckStatusValueLabel->setFormat(QString::number(clamped, 'f', 1) + tr(" dB"));
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
            if (!transition)
                transition.reset(getTransition("movit.overlay"));
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

void TrackPropertiesWidget::on_duckThresholdSpinBox_valueChanged(double value)
{
    QScopedPointer<Mlt::Transition> transition(getTransition("mix"));
    if (transition && transition->is_valid()) {
        auto command
            = new Timeline::ChangeTransitionPropertyCommand(transition->get_b_track(),
                                                            MIX_PROPERTY_DUCK_THRESHOLD,
                                                            value,
                                                            tr("Change track duck threshold"));
        connect(command, SIGNAL(valueChanged(double)), SLOT(onDuckThresholdChanged(double)));
        MAIN.undoStack()->push(command);
    }
}

void TrackPropertiesWidget::on_duckAttenuationSpinBox_valueChanged(double value)
{
    QScopedPointer<Mlt::Transition> transition(getTransition("mix"));
    if (transition && transition->is_valid()) {
        auto command
            = new Timeline::ChangeTransitionPropertyCommand(transition->get_b_track(),
                                                            MIX_PROPERTY_DUCK_ATTENUATION,
                                                            value,
                                                            tr("Change track duck attenuation"));
        connect(command, SIGNAL(valueChanged(double)), SLOT(onDuckAttenuationChanged(double)));
        MAIN.undoStack()->push(command);
    }
}

void TrackPropertiesWidget::on_duckFadeInSpinBox_valueChanged(double value)
{
    QScopedPointer<Mlt::Transition> transition(getTransition("mix"));
    if (transition && transition->is_valid()) {
        auto command
            = new Timeline::ChangeTransitionPropertyCommand(transition->get_b_track(),
                                                            MIX_PROPERTY_DUCK_FADE_IN,
                                                            value,
                                                            tr("Change track duck fade in"));
        connect(command, SIGNAL(valueChanged(double)), SLOT(onDuckFadeInChanged(double)));
        MAIN.undoStack()->push(command);
    }
}

void TrackPropertiesWidget::on_duckFadeOutSpinBox_valueChanged(double value)
{
    QScopedPointer<Mlt::Transition> transition(getTransition("mix"));
    if (transition && transition->is_valid()) {
        auto command
            = new Timeline::ChangeTransitionPropertyCommand(transition->get_b_track(),
                                                            MIX_PROPERTY_DUCK_FADE_OUT,
                                                            value,
                                                            tr("Change track duck fade out"));
        connect(command, SIGNAL(valueChanged(double)), SLOT(onDuckFadeOutChanged(double)));
        MAIN.undoStack()->push(command);
    }
}

void TrackPropertiesWidget::onDuckThresholdChanged(double value)
{
    ui->duckThresholdSpinBox->blockSignals(true);
    ui->duckThresholdSpinBox->setValue(value);
    ui->duckThresholdSpinBox->blockSignals(false);
}

void TrackPropertiesWidget::onDuckAttenuationChanged(double value)
{
    ui->duckAttenuationSpinBox->blockSignals(true);
    ui->duckAttenuationSpinBox->setValue(value);
    ui->duckAttenuationSpinBox->blockSignals(false);
}

void TrackPropertiesWidget::onDuckFadeInChanged(double value)
{
    ui->duckFadeInSpinBox->blockSignals(true);
    ui->duckFadeInSpinBox->setValue(value);
    ui->duckFadeInSpinBox->blockSignals(false);
}

void TrackPropertiesWidget::onDuckFadeOutChanged(double value)
{
    ui->duckFadeOutSpinBox->blockSignals(true);
    ui->duckFadeOutSpinBox->setValue(value);
    ui->duckFadeOutSpinBox->blockSignals(false);
}

void TrackPropertiesWidget::refreshDuckStatus()
{
    QScopedPointer<Mlt::Transition> transition(getTransition("mix"));
    if (transition && transition->is_valid()) {
        updateDuckStatus(transition->get_double(MIX_PROPERTY_DUCK_LEVEL));
    }
}
