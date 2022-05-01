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

#ifndef SAVEIMAGEDIALOG_H
#define SAVEIMAGEDIALOG_H

#include <QFileDialog>
#include <QImage>
#include <QString>

class SaveImageDialog : public QFileDialog
{
    Q_OBJECT

public:
    explicit SaveImageDialog(QWidget *parent, const QString &caption, QImage &image);
    QString saveFile()
    {
        return m_saveFile;
    };

private slots:
    void onFilterSelected(const QString &filter);
    void onFileSelected(const QString &file);

private:
    QImage &m_image;
    QString m_saveFile;
};

#endif // SAVEIMAGEDIALOG_H
