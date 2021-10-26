/*
 * Copyright (c) 2021 Meltytech, LLC
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

#include "editmarkerdialog.h"

#include "Logger.h"
#include "widgets/editmarkerwidget.h"

#include <QDebug>
#include <QDialogButtonBox>
#include <QVBoxLayout>

EditMarkerDialog::EditMarkerDialog(QWidget *parent, const QString& text, const QColor& color, int start, int end, int maxEnd)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Marker"));

    QVBoxLayout* VLayout = new QVBoxLayout(this);

    m_sWidget = new EditMarkerWidget(this, text, color, start, end, maxEnd);
    VLayout->addWidget(m_sWidget);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    VLayout->addWidget(m_buttonBox);
    connect(m_buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));

    setLayout(VLayout);
    setModal(true);
    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

QString EditMarkerDialog::getText()
{
    return m_sWidget->getText();
}

QColor EditMarkerDialog::getColor()
{
    return m_sWidget->getColor();
}

int EditMarkerDialog::getStart()
{
    return m_sWidget->getStart();
}

int EditMarkerDialog::getEnd()
{
    return m_sWidget->getEnd();
}

void EditMarkerDialog::clicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole role = m_buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::AcceptRole)
    {
        accept();
    }
    else if (role == QDialogButtonBox::RejectRole)
    {
        reject();
    }
    else
    {
        LOG_DEBUG() << "Unknown role" << role;
    }
}
