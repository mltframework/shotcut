/*
 * Copyright (c) 2019 Meltytech, LLC
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

#include "MltProducer.h"

#include <QDateTimeEdit>
#include <QDebug>
#include <QDialogButtonBox>
#include <QVBoxLayout>

FileDateDialog::FileDateDialog(QString title, Mlt::Producer* producer, QWidget* parent)
    : QDialog(parent)
    , m_producer(producer)
    , m_dtEdit(new QDateTimeEdit())
{
    setWindowTitle(tr("%1 File Date").arg(title));
    int64_t milliseconds = producer->get_creation_time();
    QDateTime creation_time;
    if ( !milliseconds ) {
        creation_time = QDateTime::currentDateTime();
    } else {
        // Set the date to the current producer date.
        creation_time = QDateTime::fromMSecsSinceEpoch(milliseconds);
    }

    m_dtEdit->setCalendarPopup(true);
    m_dtEdit->setDateTime(creation_time);

    QVBoxLayout *VLayout = new QVBoxLayout(this);
    VLayout->addWidget(m_dtEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
    VLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    this->setLayout (VLayout);
    this->setModal(true);
}

void FileDateDialog::accept()
{
    m_producer->set_creation_time((int64_t)m_dtEdit->dateTime().toTimeSpec(Qt::LocalTime).toMSecsSinceEpoch());
    QDialog::accept();
}
