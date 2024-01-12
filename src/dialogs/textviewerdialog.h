/*
 * Copyright (c) 2012-2024 Meltytech, LLC
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

#ifndef TEXTVIEWERDIALOG_H
#define TEXTVIEWERDIALOG_H

#include <QDialog>

class QDialogButtonBox;

namespace Ui {
class TextViewerDialog;
}

class TextViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TextViewerDialog(QWidget *parent = 0, bool forMltXml = false);
    ~TextViewerDialog();
    void setText(const QString &s, bool scroll = false);
    QDialogButtonBox *buttonBox() const;

private slots:
    void on_buttonBox_accepted();

private:
    Ui::TextViewerDialog *ui;
    bool m_forMltXml;
};

#endif // TEXTVIEWERDIALOG_H
