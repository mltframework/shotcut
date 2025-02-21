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

#ifndef FONTDIALOG_H
#define FONTDIALOG_H

#include <QFont>
#include <QObject>

class FontDialog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QFont selectedFont READ selectedFont WRITE setSelectedFont NOTIFY selectedFontChanged)

public:
    FontDialog(QObject *parent = nullptr);

    Q_INVOKABLE void open();

signals:
    void accepted();
    void rejected();
    void selectedFontChanged(const QFont &font);

private:
    QFont m_font;

    QFont selectedFont() const { return m_font; }
    void setSelectedFont(const QFont &font);
};

#endif // FONTDIALOG_H
