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

#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <QFileDialog>

class FileDialog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FileDialog::FileMode fileMode READ fileMode WRITE setFileMode NOTIFY fileModeChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(
        QStringList nameFilters READ nameFilters WRITE setNameFilters NOTIFY nameFiltersChanged)
    Q_PROPERTY(QString selectedFile READ selectedFile NOTIFY fileSelected)

public:
    enum FileMode { OpenFile, SaveFile };
    Q_ENUM(FileMode)

    explicit FileDialog(QObject *parent = nullptr);
    FileDialog::FileMode fileMode() const { return m_fileMode; }
    void setFileMode(FileDialog::FileMode mode);
    QString title() const;
    void setTitle(const QString &title);
    QStringList nameFilters() const;
    void setNameFilters(const QStringList &filters);
    QString selectedFile();
    Q_INVOKABLE void open();

signals:
    void fileModeChanged();
    void titleChanged();
    void nameFiltersChanged();
    void fileSelected(const QString &file);
    void filterSelected(const QString &filter);
    void accepted();
    void rejected();

private:
    FileDialog::FileMode m_fileMode{FileDialog::OpenFile};
    std::unique_ptr<QFileDialog> m_fileDialog;
};

#endif // FILEDIALOG_H
