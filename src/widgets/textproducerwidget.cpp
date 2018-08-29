/*
 * Copyright (c) 2018 Meltytech, LLC
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
#include "textproducerwidget.h"
#include "ui_textproducerwidget.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include <MltProducer.h>
#include <MltFilter.h>
#include <MltProfile.h>

static const QString kTransparent = QObject::tr("transparent", "Open Other > Text");

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

TextProducerWidget::TextProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TextProducerWidget)
{
    ui->setupUi(this);
    ui->colorLabel->setText(kTransparent);
    Util::setColorsToHighlight(ui->label_2);
    ui->preset->saveDefaultPreset(getPreset());
    ui->preset->loadPresets();
}

TextProducerWidget::~TextProducerWidget()
{
    delete ui;
}

void TextProducerWidget::on_colorButton_clicked()
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

Mlt::Producer* TextProducerWidget::newProducer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, "color:");
    p->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
    p->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
    p->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
    Mlt::Filter filter(profile, "dynamictext");
    filter.set(kShotcutFilterProperty, "dynamicText");
    if (!ui->plainTextEdit->toPlainText().isEmpty())
        filter.set("argument", ui->plainTextEdit->toPlainText().toUtf8().constData());
    else
        filter.set("argument", tr("Edit your text using the Filters panel.").toUtf8().constData());
#ifdef Q_OS_WIN
    filter.set("family", "Verdana");
#endif
    filter.set("fgcolour", "#ffffffff");
    filter.set("bgcolour", "#00000000");
    filter.set("olcolour", "#ff000000");
    filter.set("weight", 500);
    filter.set("style", "normal");
    filter.set("shotcut:usePointSize", 0);
    filter.set("size", profile.height());
    filter.set("geometry", QString("0 0 %1 %2 1").arg(profile.width()).arg(profile.height()).toUtf8().constData());
    filter.set("valign", "bottom");
    filter.set("halign", "center");
    filter.set("shotcut:animIn", "00:00:00.000");
    filter.set("shotcut:animOut", "00:00:00.000");
    filter.set_in_and_out(p->get_in(), p->get_out());
    p->attach(filter);
    return p;
}

Mlt::Properties TextProducerWidget::getPreset() const
{
    Mlt::Properties p;
    QString color = colorStringToResource(ui->colorLabel->text());
    p.set("resource", color.toLatin1().constData());
    p.set("text", ui->plainTextEdit->toPlainText().toUtf8().constData());
    return p;
}

void TextProducerWidget::loadPreset(Mlt::Properties& p)
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

void TextProducerWidget::on_preset_selected(void* p)
{
    Mlt::Properties* properties = (Mlt::Properties*) p;
    loadPreset(*properties);
    delete properties;
}

void TextProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}
