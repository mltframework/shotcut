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

#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include <QColor>
#include <QObject>

class ColorDialog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
        QColor selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

public:
    explicit ColorDialog(QObject *parent = nullptr);

    Q_INVOKABLE void open();

signals:
    void selectedColorChanged(const QColor &color);
    void accepted();
    void titleChanged();

private:
    QColor m_color;
    QString m_title;

    QColor selectedColor() const { return m_color; }
    void setSelectedColor(const QColor &color);
    QString title() const { return m_title; }
    void setTitle(const QString &title);
};

#endif // COLORDIALOG_H
