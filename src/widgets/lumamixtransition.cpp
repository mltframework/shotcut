/*
 * Copyright (c) 2014-2023 Meltytech, LLC
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

#include "lumamixtransition.h"
#include "ui_lumamixtransition.h"
#include "settings.h"
#include "mltcontroller.h"
#include "util.h"
#include "widgets/producerpreviewwidget.h"
#include "qmltypes/qmlapplication.h"

#include <QFileDialog>
#include <QFileInfo>
#include <Logger.h>

static const int kLumaComboDissolveIndex = 0;
static const int kLumaComboCutIndex = 1;
static const int kLumaComboCustomIndex = 2;

LumaMixTransition::LumaMixTransition(Mlt::Producer &producer, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LumaMixTransition)
    , m_producer(producer)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->label_2);
    m_maxStockIndex = ui->lumaCombo->count() - 1;

    // Load the wipes in AppDataDir/wipes
    for (auto &s : QmlApplication::wipes()) {
        auto i = new QListWidgetItem(QFileInfo(s).fileName());
        i->setData(Qt::UserRole, s);
        ui->lumaCombo->addItem(i);
    }
    for (int i = 0; i < ui->lumaCombo->count(); ++i) {
        auto item = ui->lumaCombo->item(i);
        item->setToolTip(item->text());
    }

    QScopedPointer<Mlt::Transition> transition(getTransition("luma"));
    if (transition && transition->is_valid()) {
        QString resource = transition->get("resource");
        ui->lumaCombo->blockSignals(true);
        if (!resource.isEmpty() && resource.indexOf("%luma") != -1) {
            ui->lumaCombo->setCurrentRow(resource.mid(resource.indexOf("%luma") + 5).left(2).toInt() + 2);
        } else if (!resource.isEmpty() && resource.startsWith("color:")) {
            ui->lumaCombo->setCurrentRow(kLumaComboCutIndex);
            ui->softnessLabel->setText(tr("Position"));
            ui->softnessSlider->setValue(qRound(QColor(resource.mid(6)).redF() * 100.0));
            ui->invertCheckBox->setDisabled(true);
        } else if (!resource.isEmpty()) {
            for (int i = m_maxStockIndex + 1; i < ui->lumaCombo->count(); ++i) {
                if (ui->lumaCombo->item(i)->data(Qt::UserRole).toString() == resource) {
                    ui->lumaCombo->setCurrentRow(i);
                    break;
                }
            }
            if (ui->lumaCombo->currentRow() < 0) {
                ui->lumaCombo->blockSignals(true);
                ui->lumaCombo->setCurrentRow(kLumaComboCustomIndex);
                ui->lumaCombo->blockSignals(false);
            }
        } else {
            ui->lumaCombo->setCurrentRow(kLumaComboDissolveIndex);
            ui->invertCheckBox->setDisabled(true);
            ui->softnessSlider->setDisabled(true);
            ui->softnessSpinner->setDisabled(true);
        }
        ui->lumaCombo->blockSignals(false);
        ui->invertCheckBox->setChecked(transition->get_int("invert"));
        if (transition->get("softness") && !resource.startsWith("color:"))
            ui->softnessSlider->setValue(qRound(transition->get_double("softness") * 100.0));
        updateCustomLumaLabel(*transition);
    }
    transition.reset(getTransition("mix"));
    if (transition && transition->is_valid()) {
        if (transition->get_int("start") == -1) {
            ui->crossfadeRadioButton->setChecked(true);
            ui->mixSlider->setDisabled(true);
            ui->mixSpinner->setDisabled(true);
        } else {
            ui->mixRadioButton->setChecked(true);
        }
        ui->mixSlider->setValue(qRound(transition->get_double("start") * 100.0));
    }
    ui->previewCheckBox->setChecked(Settings.timelinePreviewTransition());
    m_preview = new ProducerPreviewWidget(MLT.profile().dar(), 180);
    m_preview->setLooping(false);
    if (Settings.playerGPU())
        m_preview->showText(tr("Preview Not Available"));
    ui->horizontalLayout->addWidget(m_preview, 0, Qt::AlignCenter);
    connect(this, SIGNAL(modified()), this, SLOT(startPreview()), Qt::QueuedConnection);
    ui->getCustomLabel->setText(
        QString::fromLatin1("<a href=\"https://shotcut.org/resources/#transitions\">%1</a>").arg(
            ui->getCustomLabel->text()));
}

LumaMixTransition::~LumaMixTransition()
{
    m_preview->stop();
    delete ui;
}

void LumaMixTransition::onPlaying()
{
    if (m_preview) {
        m_preview->stop(false);
    }
}

void LumaMixTransition::on_invertCheckBox_clicked(bool checked)
{
    QScopedPointer<Mlt::Transition> transition(getTransition("luma"));
    if (transition && transition->is_valid()) {
        transition->set("invert", checked);
        MLT.refreshConsumer();
        emit modified();
    }
}

static void setColor(Mlt::Transition *transition, int value)
{
    qreal r = qreal(value) / 100.0;
    QColor color = QColor::fromRgbF(r, r, r);
    QString resource = QString("color:%1").arg(color.name());
    transition->set("resource", resource.toLatin1().constData());
}

void LumaMixTransition::on_softnessSlider_valueChanged(int value)
{
    QScopedPointer<Mlt::Transition> transition(getTransition("luma"));
    if (transition && transition->is_valid()) {
        if (kLumaComboCutIndex == ui->lumaCombo->currentRow()) {
            setColor(transition.data(), value);
        } else {
            transition->set("softness", value / 100.0);
        }
        MLT.refreshConsumer();
        emit modified();
    }
}

void LumaMixTransition::on_crossfadeRadioButton_clicked()
{
    QScopedPointer<Mlt::Transition> transition(getTransition("mix"));
    if (transition && transition->is_valid()) {
        transition->set("start", -1);
    }
    ui->mixSlider->setDisabled(true);
    ui->mixSpinner->setDisabled(true);
}

void LumaMixTransition::on_mixRadioButton_clicked()
{
    QScopedPointer<Mlt::Transition> transition(getTransition("mix"));
    if (transition && transition->is_valid()) {
        transition->set("start", ui->mixSlider->value() / 100.0);
    }
    ui->mixSlider->setEnabled(true);
    ui->mixSpinner->setEnabled(true);
}

void LumaMixTransition::on_mixSlider_valueChanged(int value)
{
    QScopedPointer<Mlt::Transition> transition(getTransition("mix"));
    if (transition && transition->is_valid()) {
        transition->set("start", value / 100.0);
    }
}

Mlt::Transition *LumaMixTransition::getTransition(const QString &name)
{
    QScopedPointer<Mlt::Service> service(m_producer.producer());
    while (service && service->is_valid()) {
        if (service->type() == mlt_service_transition_type) {
            Mlt::Transition transition(*service);
            if (name == transition.get("mlt_service"))
                return new Mlt::Transition(transition);
            else if (name == "luma" && QString("movit.luma_mix") == transition.get("mlt_service"))
                return new Mlt::Transition(transition);
        }
        service.reset(service->producer());
    }
    return nullptr;
}

void LumaMixTransition::updateCustomLumaLabel(Mlt::Transition &transition)
{
    ui->customLumaLabel->hide();
    ui->favoriteButton->hide();
    ui->customLumaLabel->setToolTip(QString());
    QString resource = transition.get("resource");
    if (resource.isEmpty() || resource.indexOf("%luma") != -1 || resource.startsWith("color:")
            || ui->lumaCombo->currentRow() > m_maxStockIndex) {
    } else if (!resource.isEmpty() && !resource.startsWith("color:")) {
        ui->customLumaLabel->setText(QFileInfo(transition.get("resource")).fileName());
        ui->customLumaLabel->setToolTip(transition.get("resource"));
        ui->customLumaLabel->show();
        ui->favoriteButton->show();
    }
}

void LumaMixTransition::on_lumaCombo_currentRowChanged(int index)
{
    if (index == kLumaComboDissolveIndex || index == kLumaComboCutIndex) {
        on_invertCheckBox_clicked(false);
        ui->invertCheckBox->setChecked(false);
    }
    ui->invertCheckBox->setEnabled( index != kLumaComboDissolveIndex && index != kLumaComboCutIndex);
    ui->softnessSlider->setEnabled( index != kLumaComboDissolveIndex);
    ui->softnessSpinner->setEnabled(index != kLumaComboDissolveIndex);

    QScopedPointer<Mlt::Transition> transition(getTransition("luma"));
    if (transition && transition->is_valid()) {
        if (index == kLumaComboDissolveIndex) {
            transition->set("resource", "");
            ui->softnessLabel->setText(tr("Softness"));
            transition->set("softness", ui->softnessSlider->value() / 100.0);
        } else if (index == kLumaComboCutIndex) { // Cut
            ui->softnessLabel->setText(tr("Position"));
            setColor(transition.data(), ui->softnessSlider->value());
        } else if (index == kLumaComboCustomIndex) {
            ui->softnessLabel->setText(tr("Softness"));
            // Custom file
            QString path = Settings.openPath();
#ifdef Q_OS_MAC
            path.append("/*");
#endif
            QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), path,
                                                            QString(), nullptr, Util::getFileDialogOptions());
            activateWindow();
            if (!filename.isEmpty()) {
                transition->set("resource", filename.toUtf8().constData());
                Util::getHash(*transition);
                Settings.setOpenPath(QFileInfo(filename).path());
            }
        } else if (index > m_maxStockIndex) {
            // Custom file in app data dir
            auto filename = ui->lumaCombo->item(index)->data(Qt::UserRole).toString();
            ui->softnessLabel->setText(tr("Softness"));
            transition->set("resource", filename.toUtf8().constData());
            Util::getHash(*transition);
        } else {
            ui->softnessLabel->setText(tr("Softness"));
            transition->set("resource", QString("%luma%1.pgm").arg(index - 2, 2, 10,
                                                                   QChar('0')).toLatin1().constData());
        }
        if (qstrcmp(transition->get("resource"), "")) {
            transition->set("progressive", 1);
            if (index == kLumaComboCutIndex) {
                transition->set("invert", 0);
                transition->set("softness", 0);
            } else {
                transition->set("invert", ui->invertCheckBox->isChecked());
                transition->set("softness", ui->softnessSlider->value() / 100.0);
            }
        }
        updateCustomLumaLabel(*transition);
        MLT.refreshConsumer();
        emit modified();
    }
}

void LumaMixTransition::startPreview()
{
    if (Settings.timelinePreviewTransition() && m_producer.is_valid() && MLT.isPaused()) {
        m_preview->stop();
        m_previewProducer = new Mlt::Producer(MLT.profile(), "xml-string",
                                              MLT.XML(&m_producer).toUtf8().constData());
        m_preview->start(m_previewProducer);
    }
}

void LumaMixTransition::on_previewCheckBox_clicked(bool checked)
{
    Settings.setTimelinePreviewTransition(checked);
    if (checked) {
        startPreview();
    }
}


void LumaMixTransition::on_favoriteButton_clicked()
{
    QmlApplication::addWipe(ui->customLumaLabel->toolTip());
    const auto transitions = QString::fromLatin1("transitions");
    QDir dir(Settings.appDataLocation());
    if (!dir.exists(transitions)) {
        dir.mkdir(transitions);
    }
}

