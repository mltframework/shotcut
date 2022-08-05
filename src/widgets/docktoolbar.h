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

#ifndef DOCKTOOLBAR_H
#define DOCKTOOLBAR_H

#include <QToolBar>

class DockToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit DockToolBar(const QString &title, QWidget *parent = nullptr);
    void setAreaHint(Qt::ToolBarArea area);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private slots:
    void updateStyle();

private:
    Qt::ToolBarArea m_area;
};

#endif // DOCKTOOLBAR_H
