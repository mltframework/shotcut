/*
 * Copyright (c) 2013 Meltytech, LLC
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

#ifndef PANEL_H
#define PANEL_H

#include <QWidget>

class QVBoxLayout;
class QLabel;

class Panel : public QWidget
{
    Q_OBJECT
public:
    explicit Panel(const QString& title, QWidget *parent = 0);
    void setWidget(QWidget *widget);
    QWidget *widget() const;

signals:
    void closed(bool checked);

protected:
    virtual bool event(QEvent *event);

private:
    QVBoxLayout *m_layout;
    QLabel *m_label;

    void updateStylesheet();
};

#endif // PANEL_H
