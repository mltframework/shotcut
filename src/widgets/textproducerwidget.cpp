/*
 * Copyright (c) 2018-2023 Meltytech, LLC
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
#include <QFont>
#include "textproducerwidget.h"
#include "ui_textproducerwidget.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"

static const QString kTransparent = QObject::tr("transparent", "Open Other > Color");
static const char *kSimpleFilterName = "dynamicText";
static const char *kRichFilterName = "richText";
static const int kPointSize = 60;

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

TextProducerWidget::TextProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TextProducerWidget)
{
    ui->setupUi(this);
    ui->colorLabel->setText(kTransparent);
    Util::setColorsToHighlight(ui->label_2);
    ui->preset->saveDefaultPreset(getPreset());
    Mlt::Properties p;
    p.set("resource", "#FF000000");
    ui->preset->savePreset(p, tr("black"));
    p.set("resource", "#00000000");
    ui->preset->savePreset(p, tr("transparent"));
    ui->preset->loadPresets();
}

TextProducerWidget::~TextProducerWidget()
{
    delete ui;
}

void TextProducerWidget::on_colorButton_clicked()
{
    QColor color = colorStringToResource(ui->colorLabel->text());
    if (m_producer) {
        color = QColor(QFileInfo(m_producer->get("resource")).baseName());
    }
    auto newColor = QColorDialog::getColor(color, this, QString(), QColorDialog::ShowAlphaChannel);
    if (newColor.isValid() && newColor != color) {
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
            m_producer->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
            m_producer->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
            emit producerChanged(m_producer.data());
        }
    }
}

Mlt::Producer *TextProducerWidget::newProducer(Mlt::Profile &profile)
{
    Mlt::Producer *p = new Mlt::Producer(profile, "color:");
    p->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
    p->set("mlt_image_format", "rgba");
    MLT.setDurationFromDefault(p);
    p->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
    p->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
    QScopedPointer<Mlt::Filter> filter(createFilter(profile, p));
    p->attach(*filter);
    return p;
}

Mlt::Properties TextProducerWidget::getPreset() const
{
    Mlt::Properties p;
    QString color = colorStringToResource(ui->colorLabel->text());
    p.set("resource", color.toLatin1().constData());
    if (ui->richRadioButton->isChecked()) {
        p.set("html", ui->plainTextEdit->toPlainText().toUtf8().constData());
    } else {
        p.set("argument", ui->plainTextEdit->toPlainText().toUtf8().constData());
    }
    return p;
}

void TextProducerWidget::loadPreset(Mlt::Properties &p)
{
    QColor color(QFileInfo(p.get("resource")).baseName());
    ui->colorLabel->setText(colorToString(color));
    ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
                                  .arg(Util::textColor(color), color.name()));
    if (qstrcmp("", p.get("html"))) {
        ui->plainTextEdit->setPlainText(QString::fromUtf8(p.get("html")));
        ui->richRadioButton->setChecked(true);
    } else {
        ui->plainTextEdit->setPlainText(QString::fromUtf8(p.get("argument")));
        ui->simpleRadioButton->setChecked(true);
    }
    if (m_producer) {
        m_producer->set("resource", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
        m_producer->set(kShotcutCaptionProperty, ui->colorLabel->text().toLatin1().constData());
        m_producer->set(kShotcutDetailProperty, ui->colorLabel->text().toLatin1().constData());
        QScopedPointer<Mlt::Filter> filter;
        filter.reset(MLT.getFilter(kSimpleFilterName, m_producer.data()));
        if (filter && filter->is_valid())
            m_producer->detach(*filter);
        filter.reset(MLT.getFilter(kRichFilterName, m_producer.data()));
        if (filter && filter->is_valid())
            m_producer->detach(*filter);
        Mlt::Profile profile = m_producer->get_profile();
        filter.reset(createFilter(profile, m_producer.data()));
        m_producer->attach(*filter);
        emit producerChanged(m_producer.data());
    }
}

void TextProducerWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void TextProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

Mlt::Filter *TextProducerWidget::createFilter(Mlt::Profile &profile, Mlt::Producer *p)
{
    Mlt::Filter *filter = nullptr;
    if (ui->richRadioButton->isChecked()) {
        filter = new Mlt::Filter(profile, "qtext");
        filter->set(kShotcutFilterProperty, kRichFilterName);
        QString text = ui->plainTextEdit->toPlainText();
        if (text.isEmpty())
            text = tr("Edit your text using the Filters panel.");
        QString html = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN"
                       "http://www.w3.org/TR/REC-html40/strict.dtd\">"
                       "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">"
                       "p, li { white-space: pre-wrap; }"
                       "</style></head><body style=\" font-family:'Verdana'; font-size:11pt; font-weight:400; font-style:normal;\">"
                       "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:72pt; font-weight:600; color:#ffffff;\">%1</span></p></body></html>";
        html = html.arg(text);
        filter->set("html", html.toUtf8().constData());
    } else {
        filter = new Mlt::Filter(profile, "dynamictext");
        filter->set(kShotcutFilterProperty, kSimpleFilterName);
        if (!ui->plainTextEdit->toPlainText().isEmpty())
            filter->set("argument", ui->plainTextEdit->toPlainText().toUtf8().constData());
        else
            filter->set("argument", tr("Edit your text using the Filters panel.").toUtf8().constData());
    }
#ifdef Q_OS_WIN
    filter->set("family", "Verdana");
#endif
    filter->set("fgcolour", "#ffffffff");
    filter->set("bgcolour", "#00000000");
    filter->set("olcolour", "#aa000000");
    filter->set("outline", 3);
    filter->set("weight", QFont::Bold * 10);
    filter->set("style", "normal");
    filter->set("shotcut:usePointSize", 1);
    filter->set("shotcut:pointSize", kPointSize);
    QFont font(filter->get("family"), kPointSize, filter->get_int("weight"));
    filter->set("size", QFontInfo(font).pixelSize());
    filter->set("geometry", QString("0 %1 %2 %3 1")
                .arg(qRound(0.75 * profile.height()))
                .arg(profile.width())
                .arg(profile.height() * 0.25)
                .toUtf8().constData());
    filter->set("valign", "top");
    filter->set("halign", "center");
    filter->set_in_and_out(p->get_in(), p->get_out());
    return filter;
}
