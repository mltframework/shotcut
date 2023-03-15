/*
 * Copyright (c) 2016-2022 Meltytech, LLC
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
#include "countproducerwidget.h"
#include "ui_countproducerwidget.h"
#include "util.h"
#include "mltcontroller.h"
#include <MltProfile.h>

void setLength(Mlt::Properties *p, int length)
{
    p->set("length", p->frames_to_time(length, mlt_time_clock));
    p->set("out", length - 1);
    p->set("in", 0);
}

CountProducerWidget::CountProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CountProducerWidget)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->nameLabel);

    ui->directionCombo->addItem(tr("Down"), QVariant("down"));
    ui->directionCombo->addItem(tr("Up"), QVariant("up"));
    ui->directionCombo->setCurrentIndex(0);

    ui->styleCombo->addItem(tr("Seconds"), QVariant("seconds"));
    ui->styleCombo->addItem(tr("Seconds + 1"), QVariant("seconds+1"));
    ui->styleCombo->addItem(tr("Frames"), QVariant("frames"));
    ui->styleCombo->addItem(tr("Timecode"), QVariant("timecode"));
    ui->styleCombo->addItem(tr("Clock"), QVariant("clock"));
    ui->styleCombo->setCurrentIndex(0);

    ui->soundCombo->addItem(tr("2-Pop"), QVariant("2pop"));
    ui->soundCombo->addItem(tr("Silent"), QVariant("silent"));
    ui->soundCombo->addItem(tr("Frame 0"), QVariant("frame0"));
    ui->soundCombo->setCurrentIndex(0);

    ui->backgroundCombo->addItem(tr("Clock"), QVariant("clock"));
    ui->backgroundCombo->addItem(tr("None"), QVariant("none"));
    ui->backgroundCombo->setCurrentIndex(0);

    ui->dropCheckBox->setChecked(true);

    ui->durationSpinBox->setValue(qRound(MLT.profile().fps() * 10.0)); // 10 seconds

    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
}

CountProducerWidget::~CountProducerWidget()
{
    delete ui;
}

QString CountProducerWidget::currentDirection() const
{
    return ui->directionCombo->itemData(ui->directionCombo->currentIndex()).toString();
}

QString CountProducerWidget::currentStyle() const
{
    return ui->styleCombo->itemData(ui->styleCombo->currentIndex()).toString();
}

QString CountProducerWidget::currentSound() const
{
    return ui->soundCombo->itemData(ui->soundCombo->currentIndex()).toString();
}

QString CountProducerWidget::currentBackground() const
{
    return ui->backgroundCombo->itemData(ui->backgroundCombo->currentIndex()).toString();
}

Mlt::Producer *CountProducerWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "count");
    p->set("direction", currentDirection().toLatin1().constData());
    p->set("style", currentStyle().toLatin1().constData());
    p->set("sound", currentSound().toLatin1().constData());
    p->set("background", currentBackground().toLatin1().constData());
    p->set("drop", ui->dropCheckBox->isChecked());
    setLength(p, ui->durationSpinBox->value());
    p->set(kShotcutCaptionProperty, ui->nameLabel->text().toUtf8().constData());
    p->set(kShotcutDetailProperty, detail().toUtf8().constData());
    return p;
}

Mlt::Properties CountProducerWidget::getPreset() const
{
    Mlt::Properties p;
    p.set("direction", currentDirection().toLatin1().constData());
    p.set("style", currentStyle().toLatin1().constData());
    p.set("sound", currentSound().toLatin1().constData());
    p.set("background", currentBackground().toLatin1().constData());
    p.set("drop", ui->dropCheckBox->isChecked());
    setLength(&p, ui->durationSpinBox->value());
    return p;
}

void CountProducerWidget::loadPreset(Mlt::Properties &p)
{
    if (!p.get("direction") || !p.get("style")) return;
    int index = -1;

    index = ui->directionCombo->findData(QVariant(QString::fromLatin1(p.get("direction"))));
    ui->directionCombo->setCurrentIndex(index);

    index = ui->styleCombo->findData(QVariant(QString::fromLatin1(p.get("style"))));
    ui->styleCombo->setCurrentIndex(index);

    index = ui->soundCombo->findData(QVariant(QString::fromLatin1(p.get("sound"))));
    ui->soundCombo->setCurrentIndex(index);

    index = ui->backgroundCombo->findData(QVariant(QString::fromLatin1(p.get("background"))));
    ui->backgroundCombo->setCurrentIndex(index);

    ui->dropCheckBox->setChecked(p.get("drop"));

    ui->durationSpinBox->setValue(p.get_int("length"));

    if (m_producer) {
        m_producer->set("direction", p.get("direction"));
        m_producer->set("style", p.get("style"));
        m_producer->set("sound", p.get("sound"));
        m_producer->set("background", p.get("background"));
        m_producer->set("drop", p.get("drop"));
        setLength(producer(), ui->durationSpinBox->value());
        m_producer->set(kShotcutDetailProperty, detail().toUtf8().constData());
        emit producerChanged(producer());
    }
}

void CountProducerWidget::on_directionCombo_activated(int /*index*/)
{
    if (m_producer) {
        m_producer->set("direction", currentDirection().toLatin1().constData());
        m_producer->set(kShotcutDetailProperty, detail().toUtf8().constData());
        emit producerChanged(producer());
    }
}

void CountProducerWidget::on_styleCombo_activated(int /*index*/)
{
    if (m_producer) {
        m_producer->set("style", currentStyle().toLatin1().constData());
        m_producer->set(kShotcutDetailProperty, detail().toUtf8().constData());
        emit producerChanged(producer());
    }
}

void CountProducerWidget::on_soundCombo_activated(int /*index*/)
{
    if (m_producer) {
        m_producer->set("sound", currentSound().toLatin1().constData());
        emit producerChanged(producer());
    }
}

void CountProducerWidget::on_backgroundCombo_activated(int /*index*/)
{
    if (m_producer) {
        m_producer->set("background", currentBackground().toLatin1().constData());
        emit producerChanged(producer());
    }
}

void CountProducerWidget::on_dropCheckBox_clicked(bool checked)
{
    if (m_producer) {
        m_producer->set("drop", checked);
        emit producerChanged(producer());
    }
}

void CountProducerWidget::on_durationSpinBox_editingFinished()
{
    if (!m_producer)
        return;
    if (ui->durationSpinBox->value() == m_producer->get_length())
        return;
    if (m_producer) {
        setLength(producer(), ui->durationSpinBox->value());
        MLT.stop();
        emit producerReopened(false);
        emit producerChanged(producer());
        MLT.seek(0);
    }
}

void CountProducerWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void CountProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

QString CountProducerWidget::detail() const
{
    return QString(tr("Count: %1 %2")).arg(ui->directionCombo->currentText()).arg(
               ui->styleCombo->currentText());
}
