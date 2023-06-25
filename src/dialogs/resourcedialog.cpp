/*
 * Copyright (c) 2023 Meltytech, LLC
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

#include "resourcedialog.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"
#include "transcodedialog.h"
#include "transcoder.h"
#include "widgets/resourcewidget.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

ResourceDialog::ResourceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Resources"));
    setSizeGripEnabled(true) ;

    QVBoxLayout *vlayout = new QVBoxLayout();
    LOG_DEBUG() << "Create resource widget";
    m_resourceWidget = new ResourceWidget(this);
    vlayout->addWidget(m_resourceWidget);

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    // Convert button
    QPushButton *convertButton = buttonBox->addButton(tr("Convert Selected"),
                                                      QDialogButtonBox::ActionRole);
    connect(convertButton, SIGNAL(pressed()), this, SLOT(convert()));
    vlayout->addWidget(buttonBox);

    setLayout(vlayout);
    resize(m_resourceWidget->width() + 4, m_resourceWidget->height());
}

void ResourceDialog::convert()
{
    TranscodeDialog dialog(
        tr("Choose an edit-friendly format below and then click OK to choose a file name. "
           "After choosing a file name, a job is created. "
           "When it is done, double-click the job to open it.\n"),
        MLT.profile().progressive(), this);
    dialog.setWindowTitle(tr("Convert..."));
    dialog.setWindowModality(QmlApplication::dialogModality());
    QList<Mlt::Producer> producers(m_resourceWidget->getSelected());
    Transcoder transcoder;
    transcoder.setProducers(producers);
    transcoder.convert(dialog);
}
