/*
 * Copyright (c) 2012-2023 Meltytech, LLC
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
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"
#include "Logger.h"

static const QString kTransparent = QObject::tr("transparent", "Open Other > Color");

static QString colorToString(const QColor &color)
{
    return (color == QColor(0, 0, 0, 0)) ? kTransparent
           : QString::asprintf("#%02X%02X%02X%02X",
                               qAlpha(color.rgba()),
                               qRed(color.rgba()),
                               qGreen(color.rgba()),
                               qBlue(color.rgba()));
}

static QString colorStringToResource(const QString &s)
{
    return (s == kTransparent) ? "#00000000" : s;
}

ColorProducerWidget::ColorProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ColorProducerWidget)
{
    ui->setupUi(this);
    m_title = ui->lineEdit->text();
    ui->colorLabel->setText(kTransparent);
    Util::setColorsToHighlight(ui->lineEdit, QPalette::Base);
    ui->preset->saveDefaultPreset(getPreset());
    Mlt::Properties p;
    p.set("resource", "#FF000000");
    ui->preset->savePreset(p, tr("black"));
    p.set("resource", "#00000000");
    ui->preset->savePreset(p, tr("transparent"));
    ui->preset->loadPresets();
    ui->notesLabel->setVisible(false);
    ui->notesTextEdit->setVisible(false);
}

ColorProducerWidget::~ColorProducerWidget()
{
    delete ui;
}

void ColorProducerWidget::on_colorButton_clicked()
{
    QColor color = colorStringToResource(ui->colorLabel->text());
    if (m_producer) {
        color = QColor(QFileInfo(m_producer->get("resource")).baseName());
    }
    QColorDialog::ColorDialogOptions flags = QColorDialog::ShowAlphaChannel;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (qEnvironmentVariableIsSet("SNAP") || qEnvironmentVariableIsSet("GNOME_SHELL_SESSION_MODE"))
        flags = flags | QColorDialog::DontUseNativeDialog;
#endif
    auto newColor = QColorDialog::getColor(color, this, QString(), flags);
    if (newColor.isValid()) {
        auto rgb = newColor;
        auto transparent = QColor(0, 0, 0, 0);
        rgb.setAlpha(color.alpha());
        if (newColor.alpha() == 0 && (rgb != color ||
                                      (newColor == transparent && color == transparent))) {
            newColor.setAlpha(255);
        }
        ui->colorLabel->setText(colorToString(newColor));
        ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
                                      .arg(Util::textColor(newColor), newColor.name()));
        if (m_producer) {
            m_producer->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
            if (ui->lineEdit->text().isEmpty() || ui->lineEdit->text() == m_title) {
                m_producer->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
            } else {
                m_producer->set(kShotcutCaptionProperty, ui->lineEdit->text().toUtf8().constData());
            }
            m_producer->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
            emit producerChanged(m_producer.data());
        }
    }
}

Mlt::Producer *ColorProducerWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "color:");
    p->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
    p->set("mlt_image_format", "rgba");
    MLT.setDurationFromDefault(p);
    if (ui->lineEdit->text().isEmpty() || ui->lineEdit->text() == m_title) {
        p->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
    } else {
        p->set(kShotcutCaptionProperty, ui->lineEdit->text().toUtf8().constData());
    }
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

void ColorProducerWidget::loadPreset(Mlt::Properties &p)
{
    QColor color(QFileInfo(p.get("resource")).baseName());
    ui->colorLabel->setText(colorToString(color));
    ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
                                  .arg(Util::textColor(color), color.name()));
    QString caption, detail;
    if (m_producer) {
        m_producer->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
        caption = m_producer->get(kShotcutCaptionProperty);
        detail = m_producer->get(kShotcutDetailProperty);

        if (caption.isEmpty() || caption == detail)
            m_producer->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
        m_producer->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
        emit producerChanged(m_producer.data());
    } else {
        caption = p.get(kShotcutCaptionProperty);
        detail = p.get(kShotcutDetailProperty);
    }
    if (caption.isEmpty() || caption == detail) {
        caption = m_title;
    }
    ui->lineEdit->setText(caption);
    ui->notesLabel->setVisible(true);
    ui->notesTextEdit->setVisible(true);
    ui->notesTextEdit->setPlainText(QString::fromUtf8(p.get(kCommentProperty)));
}

void ColorProducerWidget::rename()
{
    ui->lineEdit->setFocus();
    ui->lineEdit->selectAll();
}

void ColorProducerWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void ColorProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

void ColorProducerWidget::on_lineEdit_editingFinished()
{
    if (m_producer) {
        const auto caption = ui->lineEdit->text();
        if (caption.isEmpty()) {
            m_producer->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
            ui->lineEdit->setText(m_title);
        } else {
            m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
        }
        emit modified();
    }
}

void ColorProducerWidget::on_notesTextEdit_textChanged()
{
    if (m_producer && m_producer->is_valid()) {
        QString existing = QString::fromUtf8(m_producer->get(kCommentProperty));
        if (ui->notesTextEdit->toPlainText() != existing) {
            m_producer->set(kCommentProperty, ui->notesTextEdit->toPlainText().toUtf8().constData());
            emit modified();
        }
    }
}
