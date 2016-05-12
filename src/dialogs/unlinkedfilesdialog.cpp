/*
 * Copyright (c) 2016 Meltytech, LLC
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

#include "unlinkedfilesdialog.h"
#include "ui_unlinkedfilesdialog.h"
#include "settings.h"
#include "mainwindow.h"
#include "mltxmlchecker.h"
#include <QFileDialog>
#include <QStringList>

UnlinkedFilesDialog::UnlinkedFilesDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::UnlinkedFilesDialog)
{
    ui->setupUi(this);
}

UnlinkedFilesDialog::~UnlinkedFilesDialog()
{
    delete ui;
}

void UnlinkedFilesDialog::setModel(QStandardItemModel& model)
{
    QStringList headers;
    headers << tr("Missing");
    headers << tr("Replacement");
    model.setHorizontalHeaderLabels(headers);
    ui->tableView->setModel(&model);
    ui->tableView->resizeColumnsToContents();
}

void UnlinkedFilesDialog::on_tableView_doubleClicked(const QModelIndex& index)
{
    // Use File Open dialog to choose a replacement.
    QString path = Settings.openPath();
#ifdef Q_OS_MAC
    path.append("/*");
#endif
    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Open File"), path);
    if (filenames.length() > 0) {
        QAbstractItemModel* model = ui->tableView->model();

        QModelIndex firstColIndex = model->index(index.row(), MltXmlChecker::MissingColumn);
        QModelIndex secondColIndex = model->index(index.row(), MltXmlChecker::ReplacementColumn);
        QString hash = MAIN.getFileHash(filenames[0]);
        if (hash == model->data(firstColIndex, MltXmlChecker::ShotcutHashRole)) {
            // If the hashes match set icon to OK.
            QIcon icon(":/icons/oxygen/32x32/status/task-complete.png");
            model->setData(firstColIndex, icon, Qt::DecorationRole);
        } else {
            // Otherwise, set icon to warning.
            QIcon icon(":/icons/oxygen/32x32/status/task-attempt.png");
            model->setData(firstColIndex, icon, Qt::DecorationRole);
        }

        // Add chosen filename to the model.
        model->setData(secondColIndex, filenames[0]);
        model->setData(secondColIndex, filenames[0], Qt::ToolTipRole);
        model->setData(secondColIndex, hash, MltXmlChecker::ShotcutHashRole);
    }
}
