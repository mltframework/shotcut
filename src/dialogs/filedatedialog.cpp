/*
 * Copyright (c) 2019-2022 Meltytech, LLC
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

#include "filedatedialog.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "proxymanager.h"
#include "shotcut_mlt_properties.h"

#include <MltProducer.h>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QVBoxLayout>

void addDateToCombo(QComboBox *combo, const QString &description, const QDateTime &date)
{
    QDateTime local = date.toLocalTime();
    QString text = local.toString("yyyy-MM-dd HH:mm:ss") + " [" + description + "]";
    combo->addItem(text, local);
}

FileDateDialog::FileDateDialog(QString title, Mlt::Producer *producer, QWidget *parent)
    : QDialog(parent)
    , m_producer(producer)
    , m_dtCombo(new QComboBox())
    , m_dtEdit(new QDateTimeEdit())
{
    setWindowTitle(tr("%1 File Date").arg(title));
    int64_t milliseconds = producer->get_creation_time();
    QDateTime creation_time;
    if (!milliseconds) {
        creation_time = QDateTime::currentDateTime();
    } else {
        // Set the date to the current producer date.
        creation_time = QDateTime::fromMSecsSinceEpoch(milliseconds);
    }

    QVBoxLayout *VLayout = new QVBoxLayout(this);

    populateDateOptions(producer);
    m_dtCombo->setCurrentIndex(-1);
    VLayout->addWidget(m_dtCombo);
    connect(m_dtCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(dateSelected(int)));

    m_dtEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_dtEdit->setCalendarPopup(true);
    m_dtEdit->setTimeZone(QTimeZone::systemTimeZone());
    m_dtEdit->setDateTime(creation_time);
    VLayout->addWidget(m_dtEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
    VLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    this->setLayout(VLayout);
    this->setModal(true);
}

void FileDateDialog::accept()
{
    m_producer->set_creation_time(
        (int64_t) m_dtEdit->dateTime().toTimeZone(QTimeZone::systemTimeZone()).toMSecsSinceEpoch());
    QDialog::accept();
}

void FileDateDialog::dateSelected(int index)
{
    LOG_DEBUG() << index;
    if (index > -1) {
        m_dtEdit->setDateTime(m_dtCombo->itemData(index).toDateTime());
    }
}

void FileDateDialog::populateDateOptions(Mlt::Producer *producer)
{
    QDateTime dateTime;

    // Add current value
    int64_t milliseconds = producer->get_creation_time();
    if (milliseconds) {
        dateTime = QDateTime::fromMSecsSinceEpoch(milliseconds);
        addDateToCombo(m_dtCombo, tr("Current Value"), dateTime);
    }

    // Add now time
    addDateToCombo(m_dtCombo, tr("Now"), QDateTime::currentDateTime());

    // Add system info for the file.
    QString resource = ProxyManager::resource(*producer);
    QFileInfo fileInfo(resource);
    if (fileInfo.exists()) {
        addDateToCombo(m_dtCombo, tr("System - Modified"), fileInfo.lastModified());
        addDateToCombo(m_dtCombo, tr("System - Created"), fileInfo.birthTime());
    }

    // Add metadata dates
    Mlt::Producer tmpProducer(MLT.profile(), "avformat", resource.toUtf8().constData());
    if (tmpProducer.is_valid()) {
        // Standard FFMpeg creation_time
        dateTime = QDateTime::fromString(tmpProducer.get("meta.attr.creation_time.markup"),
                                         Qt::ISODateWithMs);
        if (dateTime.isValid()) {
            addDateToCombo(m_dtCombo, tr("Metadata - Creation Time"), dateTime);
        }
        // Quicktime create date
        dateTime = QDateTime::fromString(tmpProducer.get(
                                             "meta.attr.com.apple.quicktime.creationdate.markup"),
                                         Qt::ISODateWithMs);
        if (dateTime.isValid()) {
            addDateToCombo(m_dtCombo, tr("Metadata - QuickTime date"), dateTime);
        }
    }
}
