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

#include <QDockWidget>
#include <QDir>
#include <QUrl>
#include <QMap>
#include <QFuture>
#include "models/attachedfiltersmodel.h"

namespace Ui {
class FiltersDock;
}

class QActionGroup;
class QmlMetadata;
typedef QList<QAction*> QActionList;

class FiltersDock : public QDockWidget
{
    Q_OBJECT
    
public:
    explicit FiltersDock(QWidget *parent = 0);
    ~FiltersDock();
    AttachedFiltersModel* model() {
        return &m_model;
    }
    void availablefilters();
    QmlMetadata* qmlMetadataForService(Mlt::Service *service);
    void addActionToMap(const QmlMetadata *meta, QAction* action);

public slots:
    void onModelChanged();
    void onProducerOpened();
    void setFadeInDuration(int duration);
    void setFadeOutDuration(int duration);

private slots:
    void on_addAudioButton_clicked();
    
    void on_addVideoButton_clicked();
    
    void on_removeButton_clicked();
    
    void on_listView_clicked(const QModelIndex &index);
    
    void on_actionMirror_triggered();
    
    void on_listView_doubleClicked(const QModelIndex &index);
    
    void on_actionGlow_triggered();
    
    void on_actionSharpen_triggered();
    
    void on_actionColorGrading_triggered();
    
    void on_actionWhiteBalance_triggered();

    void onActionTriggered(QAction* action);

    void on_actionOverlayHTML_triggered();

private:
    Ui::FiltersDock *ui;
    AttachedFiltersModel m_model;
    QActionGroup* m_audioActions;
    QActionGroup* m_videoActions;
    QMap<QString, QAction*> m_serviceActionMap;
    QMap<QString, QAction*> m_objectNameActionMap;
    QFuture<QActionList> m_filtersFuture;
    QObject* m_quickObject;
    
    void loadWidgetsPanel(QWidget* widget = 0);
    void loadQuickPanel(const QmlMetadata *metadata, int row = -1);
};

#endif // FILTERSDOCK_H
