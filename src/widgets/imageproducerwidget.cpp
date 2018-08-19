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

#include "imageproducerwidget.h"
#include "ui_imageproducerwidget.h"
#include "settings.h"
#include "mainwindow.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include <QFileInfo>

ImageProducerWidget::ImageProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageProducerWidget),
    m_defaultDuration(-1)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->filenameLabel);
}

ImageProducerWidget::~ImageProducerWidget()
{
    delete ui;
}

Mlt::Producer* ImageProducerWidget::newProducer(Mlt::Profile& profile)
{
    Mlt::Producer* p = new Mlt::Producer(profile, m_producer->get("resource"));
    if (p->is_valid()) {
        if (ui->durationSpinBox->value() > p->get_length())
            p->set("length", ui->durationSpinBox->value());
        p->set_in_and_out(0, ui->durationSpinBox->value() - 1);
    }
    return p;
}

void ImageProducerWidget::setProducer(Mlt::Producer* p)
{
    AbstractProducerWidget::setProducer(p);
    if (m_defaultDuration == -1)
        m_defaultDuration = m_producer->get_length();
    QString s;
    if (m_producer->get(kShotcutResourceProperty))
        s = QString::fromUtf8(m_producer->get(kShotcutResourceProperty));
    else {
        s = QString::fromUtf8(m_producer->get("resource"));
        p->set("ttl", 1);
    }
    ui->filenameLabel->setText(ui->filenameLabel->fontMetrics().elidedText(s, Qt::ElideLeft, width() - 40));
    updateDuration();
    ui->resolutionLabel->setText(QString("%1x%2").arg(p->get("meta.media.width")).arg(p->get("meta.media.height")));
    ui->aspectNumSpinBox->blockSignals(true);
    if (p->get(kAspectRatioNumerator) && p->get(kAspectRatioDenominator)) {
        ui->aspectNumSpinBox->setValue(p->get_int(kAspectRatioNumerator));
        ui->aspectDenSpinBox->setValue(p->get_int(kAspectRatioDenominator));
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
    ui->aspectNumSpinBox->blockSignals(false);
    if (m_producer->get_int("ttl"))
        ui->repeatSpinBox->setValue(m_producer->get_int("ttl"));
    ui->sequenceCheckBox->setChecked(m_producer->get_int(kShotcutSequenceProperty));
    ui->repeatSpinBox->setEnabled(m_producer->get_int(kShotcutSequenceProperty));
    ui->durationSpinBox->setEnabled(!p->get(kMultitrackItemProperty));
}

void ImageProducerWidget::updateDuration()
{
    if (m_producer->get(kFilterOutProperty))
        ui->durationSpinBox->setValue(m_producer->get_int(kFilterOutProperty) - m_producer->get_int(kFilterInProperty) + 1);
    else
        ui->durationSpinBox->setValue(m_producer->get_playtime());
}

void ImageProducerWidget::reopen(Mlt::Producer* p)
{
    double speed = m_producer->get_speed();
    int position = m_producer->position();
    if (position > p->get_out())
        position = p->get_out();
    p->set("in", m_producer->get_in());
    if (MLT.setProducer(p)) {
        AbstractProducerWidget::setProducer(0);
        return;
    }
    MLT.stop();
    setProducer(p);
    emit producerReopened();
    emit producerChanged(p);
    MLT.seek(position);
    MLT.play(speed);
}

void ImageProducerWidget::recreateProducer()
{
    Mlt::Producer* p = newProducer(MLT.profile());
    p->pass_list(*m_producer, "force_aspect_ratio," kAspectRatioNumerator ", resource, " kAspectRatioDenominator
        ", ttl," kShotcutResourceProperty ", autolength, length," kShotcutSequenceProperty ", " kPlaylistIndexProperty);
    Mlt::Controller::copyFilters(*m_producer, *p);
    if (m_producer->get(kMultitrackItemProperty)) {
        emit producerChanged(p);
        delete p;
    } else {
        reopen(p);
    }
}

void ImageProducerWidget::on_resetButton_clicked()
{
    const char *s = m_producer->get(kShotcutResourceProperty);
    if (!s)
        s = m_producer->get(kShotcutResourceProperty);
    Mlt::Producer* p = new Mlt::Producer(MLT.profile(), s);
    Mlt::Controller::copyFilters(*m_producer, *p);
    if (m_producer->get(kMultitrackItemProperty)) {
        emit producerChanged(p);
        delete p;
    } else {
        reopen(p);
    }
}

void ImageProducerWidget::on_aspectNumSpinBox_valueChanged(int)
{
    if (m_producer) {
        double new_sar = double(ui->aspectNumSpinBox->value()) /
            double(ui->aspectDenSpinBox->value());
        double sar = m_producer->get_double("aspect_ratio");
        if (m_producer->get("force_aspect_ratio") || new_sar != sar) {
            m_producer->set("force_aspect_ratio", QString::number(new_sar).toLatin1().constData());
            m_producer->set(kAspectRatioNumerator, ui->aspectNumSpinBox->text().toLatin1().constData());
            m_producer->set(kAspectRatioDenominator, ui->aspectDenSpinBox->text().toLatin1().constData());
        }
        emit producerChanged(producer());
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
    if (ui->durationSpinBox->value() == m_producer->get_playtime())
        return;
    recreateProducer();
}

void ImageProducerWidget::on_sequenceCheckBox_clicked(bool checked)
{
    QString resource = m_producer->get("resource");
    ui->repeatSpinBox->setEnabled(checked);
    if (checked && !m_producer->get(kShotcutResourceProperty))
        m_producer->set(kShotcutResourceProperty, resource.toUtf8().constData());
    m_producer->set(kShotcutSequenceProperty, checked);
    m_producer->set("autolength", checked);
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
            m_producer->set("begin", begin.toLatin1().constData());
            int j = begin.toInt();
            name.replace(i, count, begin.prepend('%').append('d'));
            resource = info.path() + "/" + name;
            m_producer->set("resource", resource.toUtf8().constData());

            // Count the number of consecutive files.
            MAIN.showStatusMessage(tr("Getting length of image sequence..."));
            QCoreApplication::processEvents();
            name = info.fileName();
            name.replace(i, count, "%1");
            resource = info.path().append('/').append(name);
            for (i = j; QFile::exists(resource.arg(i, count, 10, QChar('0'))); ++i) {
                if (i % 100 == 0)
                    QCoreApplication::processEvents();
            }
            i -= j;
            m_producer->set("length", i * m_producer->get_int("ttl"));
            ui->durationSpinBox->setValue(i);
            MAIN.showStatusMessage(tr("Reloading image sequence..."));
            QCoreApplication::processEvents();
        }
    }
    else {
        m_producer->set("resource", m_producer->get(kShotcutResourceProperty));
        m_producer->set("length", qRound(MLT.profile().fps() * Mlt::kMaxImageDurationSecs));
        ui->durationSpinBox->setValue(qRound(MLT.profile().fps() * Settings.imageDuration()));
    }
    recreateProducer();
}

void ImageProducerWidget::on_repeatSpinBox_editingFinished()
{
    m_producer->set("ttl", ui->repeatSpinBox->value());
    ui->durationSpinBox->setValue(m_producer->get_length());
    MAIN.showStatusMessage(tr("Reloading image sequence..."));
    QCoreApplication::processEvents();
    recreateProducer();
}

void ImageProducerWidget::on_defaultDurationButton_clicked()
{
    Settings.setImageDuration(ui->durationSpinBox->value() / MLT.profile().fps());
}
