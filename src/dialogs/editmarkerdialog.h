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

#ifndef EDITMARKERDIALOG_H
#define EDITMARKERDIALOG_H

#include <QDialog>

class EditMarkerWidget;
class QAbstractButton;
class QDialogButtonBox;

class EditMarkerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditMarkerDialog(QWidget *parent, const QString &text, const QColor &color, int start,
                              int end, int maxEnd);
    QString getText();
    QColor getColor();
    int getStart();
    int getEnd();

private slots:
    void clicked(QAbstractButton *button);

private:
    EditMarkerWidget *m_sWidget;
    QDialogButtonBox *m_buttonBox;
};

#endif // EDITMARKERDIALOG_H
