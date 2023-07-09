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

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>

ResourceDialog::ResourceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Resources"));
    setSizeGripEnabled(true) ;

    QVBoxLayout *vlayout = new QVBoxLayout();
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
}

void ResourceDialog::search(Mlt::Producer *producer)
{
    m_resourceWidget->search(producer);
}

void ResourceDialog::add(Mlt::Producer *producer)
{
    m_resourceWidget->add(producer);
}

void ResourceDialog::selectTroubleClips()
{
    m_resourceWidget->selectTroubleClips();
}

bool ResourceDialog::hasTroubleClips()
{
    return m_resourceWidget->hasTroubleClips();
}

void ResourceDialog::convert()
{
    QList<Mlt::Producer> producers(m_resourceWidget->getSelected());

    // Only convert avformat producers
    QMutableListIterator<Mlt::Producer> i(producers);
    while (i.hasNext()) {
        Mlt::Producer producer = i.next();
        if (!QString(producer.get("mlt_service")).startsWith("avformat"))
            i.remove();
    }

    if (producers.length() < 1) {
        QMessageBox::warning(this, windowTitle(), tr("No resources to convert"));
        return;
    }

    TranscodeDialog dialog(
        tr("Choose an edit-friendly format below and then click OK to choose a file name. "
           "After choosing a file name, a job is created. "
           "When it is done, double-click the job to open it.\n"),
        MLT.profile().progressive(), this);
    dialog.setWindowTitle(tr("Convert..."));
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.set709Convert(true);
    Transcoder transcoder;
    transcoder.setProducers(producers);
    transcoder.convert(dialog);
    accept();
}

void ResourceDialog::showEvent(QShowEvent *event)
{
    m_resourceWidget->updateSize();
    resize(m_resourceWidget->width() + 4, m_resourceWidget->height());
    QDialog::showEvent(event);
}
