/*
 * Copyright (c) 2022 Meltytech, LLC
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

#ifndef NOTESDOCK_H
#define NOTESDOCK_H

#include <QDockWidget>
#include <QObject>
#include <QQuickView>
#include <QQuickWidget>

class NotesDock : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit NotesDock(QWidget *parent = 0);
    QString getText();
    void setText(const QString& text);

protected:
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent* event);

signals:
    void modified();

private slots:
    void resetQview();
    void onTextChanged(QString);
    void onMinimumWidthChanged();

private:
    QQuickWidget m_qview;
    bool m_blockUpdate;
};

#endif // NOTESDOCK_H
