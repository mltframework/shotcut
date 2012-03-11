/*
 * Copyright (c) 2012 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "imageproducerwidget.h"
#include "ui_imageproducerwidget.h"
#include <QtCore/QFileInfo>

ImageProducerWidget::ImageProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageProducerWidget),
    m_defaultDuration(-1)
{
    ui->setupUi(this);
}

ImageProducerWidget::~ImageProducerWidget()
{
    delete ui;
}

Mlt::Producer* ImageProducerWidget::producer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, m_producer->get("resource"));
    return p;
}

void ImageProducerWidget::setProducer(Mlt::Producer* p)
{
    AbstractProducerWidget::setProducer(p);
    if (m_defaultDuration == -1)
        m_defaultDuration = m_producer->get_length();
    QString s;
    if (m_producer->get("shotcut_resource"))
        s = QString::fromUtf8(m_producer->get("shotcut_resource"));
    else {
        s = QString::fromUtf8(m_producer->get("resource"));
        p->set("ttl", 1);
    }
    ui->filenameLabel->setText(ui->filenameLabel->fontMetrics().elidedText(s, Qt::ElideLeft, width() - 40));
    ui->durationSpinBox->setValue(m_producer->get_length());
    ui->widthLineEdit->setText(p->get("meta.media.width"));
    ui->heightLineEdit->setText(p->get("meta.media.height"));
    if (p->get("shotcut_aspect_num") && p->get("shotcut_aspect_den")) {
        ui->aspectNumSpinBox->setValue(p->get_int("shotcut_aspect_num"));
        ui->aspectDenSpinBox->setValue(p->get_int("shotcut_aspect_den"));
    }
    else {
        double sar = m_producer->get_double("aspect_ratio");
        if (m_producer->get("force_aspect_ratio"))
            sar = m_producer->get_double("force_aspect_ratio");
        if (sar == 1.0) {
            ui->aspectNumSpinBox->setValue(1);
            ui->aspectDenSpinBox->setValue(1);
        } else {
            ui->aspectNumSpinBox->setValue(1000 * sar);
            ui->aspectDenSpinBox->setValue(1000);
        }
    }
    if (m_producer->get_int("ttl"))
        ui->repeatSpinBox->setValue(m_producer->get_int("ttl"));
    ui->sequenceCheckBox->setChecked(m_producer->get_int("shotcut_sequence"));
}

void ImageProducerWidget::reopen(Mlt::Producer* p)
{
    int length = ui->durationSpinBox->value();
    int out = m_producer->get_out();
    int position = m_producer->position();
    double speed = m_producer->get_speed();

    p->set("length", length);
    if (out + 1 >= m_producer->get_length())
        p->set("out", length - 1);
    else if (out >= length)
        p->set("out", length - 1);
    else
        p->set("out", out);
    if (position > p->get_out())
        position = p->get_out();
    p->set("in", m_producer->get_in());
    if (MLT.open(p)) {
        setProducer(0);
        return;
    }
    emit producerReopened();
    MLT.seek(position);
    MLT.play(speed);
    setProducer(p);
}

void ImageProducerWidget::on_resetButton_clicked()
{
    const char *s = m_producer->get("shotcut_resource");
    if (!s)
        s = m_producer->get("resource");
    Mlt::Producer* p = new Mlt::Producer(MLT.profile(), s);
    reopen(p);
}

void ImageProducerWidget::on_aspectNumSpinBox_valueChanged(int)
{
    if (m_producer) {
        double new_sar = double(ui->aspectNumSpinBox->value()) /
            double(ui->aspectDenSpinBox->value());
        double sar = m_producer->get_double("aspect_ratio");
        if (m_producer->get("force_aspect_ratio") || new_sar != sar) {
            m_producer->set("force_aspect_ratio", QString::number(new_sar).toAscii().constData());
            m_producer->set("shotcut_aspect_num", ui->aspectNumSpinBox->text().toAscii().constData());
            m_producer->set("shotcut_aspect_den", ui->aspectDenSpinBox->text().toAscii().constData());
        }
        emit producerChanged();
    }
}

void ImageProducerWidget::on_aspectDenSpinBox_valueChanged(int i)
{
    on_aspectNumSpinBox_valueChanged(i);
}

void ImageProducerWidget::on_durationSpinBox_editingFinished()
{
    if (!m_producer)
        return;
    if (ui->durationSpinBox->value() == m_producer->get_length())
        return;
    Mlt::Producer* p = producer(MLT.profile());
    p->pass_list(*m_producer, "force_aspect_ratio, shotcut_aspect_num, shotcut_aspect_den,"
        "shotcut_resource, resource, ttl, shotcut_sequence");
    reopen(p);
}

void ImageProducerWidget::on_sequenceCheckBox_clicked(bool checked)
{
    QString resource = m_producer->get("resource");
    ui->repeatSpinBox->setEnabled(checked);
    if (checked && !m_producer->get("shotcut_resource"))
        m_producer->set("shotcut_resource", resource.toUtf8().constData());
    m_producer->set("shotcut_sequence", checked);
    m_producer->set("ttl", ui->repeatSpinBox->value());
    if (checked) {
        QFileInfo info(resource);
        QString name(info.fileName());
        QString begin = "";
        int i = name.length();
        int count = 0;

        // find the last numeric digit
        for (; i && !name[i - 1].isDigit(); i--) {};
        // count the digits and build the begin value
        for (; i && name[i - 1].isDigit(); i--, count++)
            begin.prepend(name[i - 1]);
        if (count) {
            m_producer->set("begin", begin.toAscii().constData());
            name.replace(i, count, begin.prepend('%').append('d'));
            resource = info.path() + "/" + name;
            m_producer->set("resource", resource.toUtf8().constData());
        }
    }
    else {
        m_producer->set("resource", m_producer->get("shotcut_resource"));
    }
    Mlt::Producer* p = producer(MLT.profile());
    p->pass_list(*m_producer, "force_aspect_ratio, shotcut_aspect_num, shotcut_aspect_den, "
        "shotcut_resource, resource, ttl, shotcut_sequence");
    reopen(p);
}

void ImageProducerWidget::on_repeatSpinBox_editingFinished()
{
    m_producer->set("ttl", ui->repeatSpinBox->value());
    emit producerChanged();
}
