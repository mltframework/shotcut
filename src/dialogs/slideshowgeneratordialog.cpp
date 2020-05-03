/*
 * Copyright (c) 2020 Meltytech, LLC
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

#include "slideshowgeneratordialog.h"

#include "Logger.h"
#include "widgets/slideshowgeneratorwidget.h"

#include <MltProfile.h>
#include <MltTransition.h>

#include <QDebug>
#include <QDialogButtonBox>
#include <QVBoxLayout>

SlideshowGeneratorDialog::SlideshowGeneratorDialog(QWidget* parent, Mlt::Playlist& clips)
    : QDialog(parent)
{
    setWindowTitle(tr("Slideshow Generator - %1 Clips").arg(QString::number(clips.count())));

    QVBoxLayout* VLayout = new QVBoxLayout(this);

    m_sWidget = new SlideshowGeneratorWidget(&clips, this);
    VLayout->addWidget(m_sWidget);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close);
    VLayout->addWidget(m_buttonBox);
    connect(m_buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));

    this->setLayout (VLayout);
    this->setModal(true);
}

Mlt::Playlist* SlideshowGeneratorDialog::getSlideshow()
{
    return m_sWidget->getSlideshow();
}

void SlideshowGeneratorDialog::clicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole role = m_buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::ApplyRole)
    {
        LOG_DEBUG() << "Apply";
        accept();
    }
    else if (role == QDialogButtonBox::RejectRole)
    {
        LOG_DEBUG() << "Reject";
        reject();
    }
    else
    {
        LOG_DEBUG() << "Unknown role" << role;
    }
}
