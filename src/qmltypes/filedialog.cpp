/*
 * Copyright (c) 2023-2026 Meltytech, LLC
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
{}

QFileDialog *FileDialog::getOrCreateDialog()
{
    if (!m_fileDialog) {
        m_fileDialog.reset(new QFileDialog(&MAIN));
        if (!m_title.isEmpty())
            m_fileDialog->setWindowTitle(m_title);
        if (!m_nameFilters.isEmpty())
            m_fileDialog->setNameFilters(m_nameFilters);
        connect(m_fileDialog.get(), &QDialog::accepted, this, &FileDialog::accepted);
        connect(m_fileDialog.get(), &QDialog::rejected, this, &FileDialog::rejected);
        connect(m_fileDialog.get(), &QFileDialog::fileSelected, this, &FileDialog::fileSelected);
        connect(m_fileDialog.get(), &QFileDialog::filterSelected, this, &FileDialog::filterSelected);
    }
    return m_fileDialog.get();
}

void FileDialog::setFileMode(FileMode mode)
{
    m_fileMode = mode;
}

QString FileDialog::title() const
{
    return m_fileDialog ? m_fileDialog->windowTitle() : m_title;
}

void FileDialog::setTitle(const QString &title)
{
    if (title != m_title) {
        m_title = title;
        if (m_fileDialog)
            m_fileDialog->setWindowTitle(title);
        emit titleChanged();
    }
}

QStringList FileDialog::nameFilters() const
{
    return m_fileDialog ? m_fileDialog->nameFilters() : m_nameFilters;
}

void FileDialog::setNameFilters(const QStringList &filters)
{
    if (filters != m_nameFilters) {
        m_nameFilters = filters;
        if (m_fileDialog)
            m_fileDialog->setNameFilters(filters);
        emit nameFiltersChanged();
    }
}

QString FileDialog::selectedFile()
{
    return m_fileDialog ? m_fileDialog->selectedFiles().first() : QString();
}

void FileDialog::open()
{
    auto *dialog = getOrCreateDialog();
    if (m_fileMode == FileDialog::OpenFile) {
        dialog->setAcceptMode(QFileDialog::AcceptOpen);
        dialog->setDirectory(Settings.openPath());
    } else {
        dialog->setAcceptMode(QFileDialog::AcceptSave);
        dialog->setDirectory(Settings.savePath());
    }
#ifdef Q_OS_MAC
    dialog->setWindowModality(Qt::NonModal);
#else
    dialog->setWindowModality(Qt::ApplicationModal);
#endif
    dialog->setOptions(Util::getFileDialogOptions());
    dialog->open();
}
