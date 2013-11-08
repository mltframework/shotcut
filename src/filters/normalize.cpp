/*
 * Copyright (c) 2013 Meltytech, LLC
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

#include "normalize.h"
#include "ui_normalize.h"
#include "jobqueue.h"
#include "mainwindow.h"
#include <QIODevice>
#include <QTemporaryFile>
#include <QFile>
#include <QtXml>
#include <QtDebug>

NormalizeFilter::NormalizeFilter(Mlt::Filter filter, bool setDefaults, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Normalize),
    m_filter(filter)
{
    ui->setupUi(this);
    ui->buttonGroup->setId(ui->peakRadioButton, 1);
    ui->buttonGroup->setId(ui->powerrRadioButton, 0);
    if (setDefaults)
        ui->levelSpinBox->setValue(-12.0);
    else
        ui->levelSpinBox->setValue(m_filter.get_double("analysis_level"));
    if (setDefaults || !m_filter.get_int("use_peak")) {
        m_filter.set("use_peak", 0);
        ui->powerrRadioButton->setChecked(true);
        ui->levelSpinBox->setEnabled(true);
    } else {
        m_filter.set("use_peak", 1);
        ui->peakRadioButton->setChecked(true);
        ui->levelSpinBox->setDisabled(true);
    }
    if (m_filter.get("effect") && QString::fromLatin1(m_filter.get("effect")).startsWith("vol "))
        ui->statusLabel->setText(tr("Analysis complete"));
    else
        ui->statusLabel->setText(tr("Click Analyze to use this filter."));
}

NormalizeFilter::~NormalizeFilter()
{
    delete ui;
}

void NormalizeFilter::on_analyzeButton_clicked()
{
    ui->analyzeButton->setEnabled(false);
    ui->statusLabel->setText("");

    // get temp filename for input xml
    QTemporaryFile tmp(QDir::tempPath().append("/shotcut-XXXXXX"));
    tmp.open();
    QString tmpName = tmp.fileName();
    tmp.close();
    tmpName.append(".mlt");
    m_filter.set("effect", "analysis");
    m_filter.set("analysis_level", ui->levelSpinBox->text().toLatin1().constData());
    MLT.saveXML(tmpName);
    m_filter.set("effect", "");

    // get temp filename for output xml
    QTemporaryFile tmpTarget(QDir::tempPath().append("/shotcut-XXXXXX"));
    tmpTarget.open();
    QString target = tmpTarget.fileName();
    tmpTarget.close();
    target.append(".mlt");

    // parse xml
    QFile f1(tmpName);
    f1.open(QIODevice::ReadOnly);
    QDomDocument dom(tmpName);
    dom.setContent(&f1);
    f1.close();

    // add consumer element
    QDomElement consumerNode = dom.createElement("consumer");
    QDomNodeList profiles = dom.elementsByTagName("profile");
    if (profiles.isEmpty())
        dom.documentElement().insertAfter(consumerNode, dom.documentElement());
    else
        dom.documentElement().insertAfter(consumerNode, profiles.at(profiles.length() - 1));
    consumerNode.setAttribute("mlt_service", "xml");
    consumerNode.setAttribute("all", 1);
    consumerNode.setAttribute("video_off", 1);
    consumerNode.setAttribute("no_meta", 1);
    consumerNode.setAttribute("resource", target);

    // save new xml
    f1.open(QIODevice::WriteOnly);
    QTextStream ts(&f1);
    dom.save(ts, 2);
    f1.close();

    MeltJob* job = new MeltJob(target, tmpName);
    if (job) {
        connect(job, SIGNAL(finished(MeltJob*,bool)), SLOT(onJobFinished(MeltJob*,bool)));
        QFileInfo info(MLT.resource());
        job->setLabel(tr("Normalize %1").arg(info.fileName()));
        JOBS.add(job);
    }
}

void NormalizeFilter::onJobFinished(MeltJob *job, bool isSuccess)
{
    QString fileName = job->objectName();

    if (isSuccess) {
        // parse the xml
        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        QDomDocument dom(fileName);
        dom.setContent(&file);
        file.close();

        QDomNodeList filters = dom.elementsByTagName("filter");
        for (int i = 0; i < filters.size(); i++) {
            QDomNode filterNode = filters.at(i);
            bool foundSox = false;

            QDomNodeList properties = filterNode.toElement().elementsByTagName("property");
            for (int j = 0; j < properties.size(); j++) {
                QDomNode propertyNode = properties.at(j);
                if (propertyNode.attributes().namedItem("name").toAttr().value() == "mlt_service"
                        && propertyNode.toElement().text() == "sox") {
                    foundSox = true;
                    break;
                }
            }
            if (foundSox) {
                for (int j = 0; j < properties.size(); j++) {
                    QDomNode propertyNode = properties.at(j);
                    if (propertyNode.attributes().namedItem("name").toAttr().value() == "effect") {
                        m_filter.set("effect", propertyNode.toElement().text().toLatin1().constData());
                        // Remove any existing sox effect instances - up to 16 channels.
                        // XXX should this be moved to MLT filter_sox.c?
                        for (int k = 0; k < 16; k++) {
                            QString s = QString("_effect_0_%1").arg(k);
                            m_filter.set(s.toLatin1().constData(), NULL, 0);
                        }
                        break;
                    }
                }
                break;
            }
        }
        ui->statusLabel->setText(tr("Analysis complete"));
    }
    ui->analyzeButton->setEnabled(true);
    QFile::remove(fileName);
}

void NormalizeFilter::on_buttonGroup_buttonClicked(int i)
{
    ui->levelSpinBox->setDisabled(i);
    if (m_filter.get_int("use_peak") != i) {
        ui->statusLabel->setText("");
        m_filter.set("use_peak", i);
    }
}

void NormalizeFilter::on_levelSpinBox_valueChanged(double value)
{
    ui->statusLabel->setText("");
}
