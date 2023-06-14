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

#include "filedialog.h"
#include "mainwindow.h"
#include "settings.h"
#include "util.h"

FileDialog::FileDialog(QObject *parent)
    : QObject{parent}
{
    m_fileDialog.reset(new QFileDialog(&MAIN));
    connect(m_fileDialog.get(), &QDialog::accepted, this, &FileDialog::accepted);
    connect(m_fileDialog.get(), &QDialog::rejected, this, &FileDialog::rejected);
    connect(m_fileDialog.get(), &QFileDialog::fileSelected, this, &FileDialog::fileSelected);
    connect(m_fileDialog.get(), &QFileDialog::filterSelected, this, &FileDialog::filterSelected);
}

void FileDialog::setFileMode(FileMode mode)
{
    m_fileMode = mode;
}

QString FileDialog::title() const
{
    return m_fileDialog->windowTitle();
}

void FileDialog::setTitle(const QString &title)
{
    if (title != m_fileDialog->windowTitle()) {
        m_fileDialog->setWindowTitle(title);
        emit titleChanged();
    }
}

QStringList FileDialog::nameFilters() const
{
    return m_fileDialog->nameFilters();
}

void FileDialog::setNameFilters(const QStringList &filters)
{
    if (filters != m_fileDialog->nameFilters()) {
        m_fileDialog->setNameFilters(filters);
        emit nameFiltersChanged();
    }
}

QString FileDialog::selectedFile()
{
    return m_fileDialog->selectedFiles().first();
}

void FileDialog::open()
{
    if (m_fileMode == FileDialog::OpenFile) {
        m_fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
        m_fileDialog->setDirectory(Settings.openPath());
    } else {
        m_fileDialog->setAcceptMode(QFileDialog::AcceptSave);
        m_fileDialog->setDirectory(Settings.savePath());
    }
#ifdef Q_OS_MAC
    m_fileDialog->setWindowModality(Qt::NonModal);
#else
    m_fileDialog->setWindowModality(Qt::ApplicationModal);
#endif
    m_fileDialog->setOptions(Util::getFileDialogOptions());
    m_fileDialog->open();
}
