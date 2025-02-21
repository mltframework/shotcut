/*
 * Copyright (c) 2016-2023 Meltytech, LLC
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
#include "Logger.h"
#include "mltxmlchecker.h"
#include "settings.h"
#include "ui_unlinkedfilesdialog.h"
#include "util.h"

#include <QFileDialog>
#include <QStringList>

UnlinkedFilesDialog::UnlinkedFilesDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UnlinkedFilesDialog)
{
    ui->setupUi(this);
}

UnlinkedFilesDialog::~UnlinkedFilesDialog()
{
    delete ui;
}

void UnlinkedFilesDialog::setModel(QStandardItemModel &model)
{
    QStringList headers;
    headers << tr("Missing");
    headers << tr("Replacement");
    model.setHorizontalHeaderLabels(headers);
    ui->tableView->setModel(&model);
    ui->tableView->resizeColumnsToContents();
}

void UnlinkedFilesDialog::on_tableView_doubleClicked(const QModelIndex &index)
{
    // Use File Open dialog to choose a replacement.
    QString path = Settings.openPath();
#ifdef Q_OS_MAC
    path.append("/*");
#endif
    QStringList filenames = QFileDialog::getOpenFileNames(this,
                                                          tr("Open File"),
                                                          path,
                                                          QString(),
                                                          nullptr,
                                                          Util::getFileDialogOptions());
    if (filenames.length() > 0) {
        QAbstractItemModel *model = ui->tableView->model();

        QModelIndex firstColIndex = model->index(index.row(), MltXmlChecker::MissingColumn);
        QModelIndex secondColIndex = model->index(index.row(), MltXmlChecker::ReplacementColumn);
        QString hash = Util::getFileHash(filenames[0]);
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
        QString filePath = QDir::toNativeSeparators(filenames[0]);
        model->setData(secondColIndex, filePath);
        model->setData(secondColIndex, filePath, Qt::ToolTipRole);
        model->setData(secondColIndex, hash, MltXmlChecker::ShotcutHashRole);

        QFileInfo fi(QFileInfo(filenames.first()));
        Settings.setOpenPath(fi.path());
        lookInDir(fi.dir());
    }
}

bool UnlinkedFilesDialog::lookInDir(const QDir &dir, bool recurse)
{
    LOG_DEBUG() << dir.canonicalPath();
    // returns true if outstanding is > 0
    unsigned outstanding = 0;
    QAbstractItemModel *model = ui->tableView->model();
    for (int row = 0; row < model->rowCount(); row++) {
        QModelIndex replacementIndex = model->index(row, MltXmlChecker::ReplacementColumn);
        if (model->data(replacementIndex, MltXmlChecker::ShotcutHashRole).isNull())
            ++outstanding;
    }
    if (outstanding) {
        for (const auto &fileName :
             dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
            QString hash = Util::getFileHash(dir.absoluteFilePath(fileName));
            for (int row = 0; row < model->rowCount(); row++) {
                QModelIndex replacementIndex = model->index(row, MltXmlChecker::ReplacementColumn);
                if (model->data(replacementIndex, MltXmlChecker::ShotcutHashRole).isNull()) {
                    QModelIndex missingIndex = model->index(row, MltXmlChecker::MissingColumn);
                    QFileInfo missingInfo(model->data(missingIndex).toString());
                    QString missingHash
                        = model->data(missingIndex, MltXmlChecker::ShotcutHashRole).toString();
                    if (hash == missingHash || fileName == missingInfo.fileName()) {
                        if (hash == missingHash) {
                            QIcon icon(":/icons/oxygen/32x32/status/task-complete.png");
                            model->setData(missingIndex, icon, Qt::DecorationRole);
                        } else {
                            QIcon icon(":/icons/oxygen/32x32/status/task-attempt.png");
                            model->setData(missingIndex, icon, Qt::DecorationRole);
                        }
                        QString filePath = QDir::toNativeSeparators(dir.absoluteFilePath(fileName));
                        model->setData(replacementIndex, filePath);
                        model->setData(replacementIndex, filePath, Qt::ToolTipRole);
                        model->setData(replacementIndex, hash, MltXmlChecker::ShotcutHashRole);
                        QCoreApplication::processEvents();
                        if (--outstanding)
                            break;
                        else
                            return false;
                    }
                }
            }
        }
    }
    if (outstanding && recurse) {
        for (const QString &dirName :
             dir.entryList(QDir::Dirs | QDir::Executable | QDir::NoDotAndDotDot)) {
            if (!lookInDir(dir.absoluteFilePath(dirName), true))
                break;
        }
    }
    return outstanding;
}

void UnlinkedFilesDialog::on_searchFolderButton_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this,
                                                        windowTitle(),
                                                        Settings.openPath(),
                                                        Util::getFileDialogOptions());
    if (!dirName.isEmpty()) {
        Settings.setOpenPath(dirName);
        lookInDir(dirName);
    }
}
