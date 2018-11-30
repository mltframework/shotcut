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

#include <QColorDialog>
#include <QFileInfo>
#include "colorproducerwidget.h"
#include "ui_colorproducerwidget.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

static const QString kTransparent = QObject::tr("transparent", "Open Other > Color");

static QString colorToString(const QColor& color)
{
    return (color.alpha() == 0) ? kTransparent
                                : QString().sprintf("#%02X%02X%02X%02X",
                                                    qAlpha(color.rgba()),
                                                    qRed(color.rgba()),
                                                    qGreen(color.rgba()),
                                                    qBlue(color.rgba()));
}

static QString colorStringToResource(const QString& s)
{
    return (s == kTransparent) ? "#00000000" : s;
}

ColorProducerWidget::ColorProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ColorProducerWidget)
{
    ui->setupUi(this);
    ui->colorLabel->setText(kTransparent);
    Util::setColorsToHighlight(ui->label_2);
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
}

ColorProducerWidget::~ColorProducerWidget()
{
    delete ui;
}

void ColorProducerWidget::on_colorButton_clicked()
{
    QColorDialog dialog;
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    if (dialog.exec() == QDialog::Accepted) {
        ui->colorLabel->setText(colorToString(dialog.currentColor()));
        ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
                                      .arg((dialog.currentColor().value() < 150)? "white":"black")
                                      .arg(dialog.currentColor().name()));
        if (m_producer) {
            m_producer->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
            m_producer->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
            m_producer->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
            emit producerChanged(m_producer.data());
        }
    }
}

Mlt::Producer* ColorProducerWidget::newProducer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, "color:");
    p->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
    p->set("mlt_image_format", "rgb24a");
    p->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
    p->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
    return p;
}

Mlt::Properties ColorProducerWidget::getPreset() const
{
    Mlt::Properties p;
    QString color = colorStringToResource(ui->colorLabel->text());
    p.set("resource", color.toLatin1().constData());
    return p;
}

void ColorProducerWidget::loadPreset(Mlt::Properties& p)
{
    QColor color(QFileInfo(p.get("resource")).baseName());
    ui->colorLabel->setText(colorToString(color));
    ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
        .arg((color.value() < 150)? "white":"black")
        .arg(color.name()));
    if (m_producer) {
        m_producer->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
        m_producer->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
        m_producer->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
        emit producerChanged(m_producer.data());
    }
}

void ColorProducerWidget::on_preset_selected(void* p)
{
    Mlt::Properties* properties = (Mlt::Properties*) p;
    loadPreset(*properties);
    delete properties;
}

void ColorProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}
