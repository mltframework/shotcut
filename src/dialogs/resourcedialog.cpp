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
    ResourceWidget *resourceWidget = new ResourceWidget(this);
    vlayout->addWidget(resourceWidget);

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    vlayout->addWidget(buttonBox);

    setLayout(vlayout);
    resize(resourceWidget->width() + 4, resourceWidget->height());
}
