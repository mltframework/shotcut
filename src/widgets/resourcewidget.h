/*
 * Copyright (c) 2023-2024 Meltytech, LLC
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

#ifndef RESOURCEWIDGET_H
#define RESOURCEWIDGET_H

#include <Mlt.h>

#include <QWidget>

class ResourceModel;
class QTreeView;

class ResourceWidget : public QWidget
{
    Q_OBJECT

public:
    ResourceWidget(QWidget *parent);
    virtual ~ResourceWidget();

    void search(Mlt::Producer *producer);
    void add(Mlt::Producer *producer);
    void selectTroubleClips();
    bool hasTroubleClips();
    int producerCount();
    Mlt::Producer producer(int index);
    QList<Mlt::Producer> getSelected();
    void updateSize();

private:
    ResourceModel *m_model;
    QTreeView *m_table;
};

#endif // RESOURCEWIDGET_H
