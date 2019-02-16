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

#include <QDebug>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QVBoxLayout>

#include <utime.h>

FileDateDialog::FileDateDialog(QString filename, QWidget* parent)
    : QDialog(parent)
    , m_filename(filename)
    , m_dtEdit(new QDateTimeEdit())
{
    QFileInfo fileInfo(m_filename);
    setWindowTitle(tr("%1 File Date").arg(fileInfo.fileName()));

    m_dtEdit->setCalendarPopup(true);
    // Set the date to the current file date.
    m_dtEdit->setDateTime(fileInfo.lastModified());

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
    // Apply the new file date to the file.
    // TODO: When QT 5.10 is available, use QFileDevice functions
#ifdef Q_OS_WIN
    struct _utimbuf dstTime;
    dstTime.actime = m_dtEdit->dateTime().toMSecsSinceEpoch() / 1000;
    dstTime.modtime = dstTime.actime;
    _utime(m_filename.toUtf8().constData(), &dstTime);
#else
    struct utimbuf dstTime;
    dstTime.actime = m_dtEdit->dateTime().toMSecsSinceEpoch() / 1000;
    dstTime.modtime = dstTime.actime;
    utime(m_filename.toUtf8().constData(), &dstTime);
#endif

    QDialog::accept();
}
