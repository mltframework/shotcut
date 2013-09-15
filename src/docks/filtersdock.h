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

#ifndef FILTERSDOCK_H
#define FILTERSDOCK_H

#include "widgets/panel.h"
#include "models/attachedfiltersmodel.h"

namespace Ui {
class FiltersDock;
}

class FiltersDock : public Panel
{
    Q_OBJECT
    
public:
    explicit FiltersDock(QWidget *parent = 0);
    ~FiltersDock();
    AttachedFiltersModel* model() {
        return &m_model;
    }

public slots:
    void onModelChanged();
    void onProducerOpened();

private slots:
    void on_addButton_clicked();
    
    void on_removeButton_clicked();
    
    void on_listView_clicked(const QModelIndex &index);
    
    void on_actionBlur_triggered();
    
    void on_actionMirror_triggered();
    
    void on_listView_doubleClicked(const QModelIndex &index);
    
    void on_actionDiffusion_triggered();
    
    void on_actionGlow_triggered();
    
    void on_actionSharpen_triggered();
    
    void on_actionVignette_triggered();
    
    void on_actionCrop_triggered();
    
    void on_actionColorGrading_triggered();
    
    void on_actionSizePosition_triggered();
    
    void on_actionSaturation_triggered();
    
    void on_actionWhiteBalance_triggered();
    
private:
    Ui::FiltersDock *ui;
    AttachedFiltersModel m_model;
    bool m_isGPU;
};

#endif // FILTERSDOCK_H
