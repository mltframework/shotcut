/*
 * Copyright (c) 2018-2024 Meltytech, LLC
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

#ifndef NEWPROJECTFOLDER_H
#define NEWPROJECTFOLDER_H

#include <QWidget>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QMenu>
#include <QString>

namespace Ui {
class NewProjectFolder;
}

class QAction;
class QActionGroup;

class NewProjectFolder : public QWidget
{
    Q_OBJECT

public:
    explicit NewProjectFolder(QWidget *parent = 0);
    ~NewProjectFolder();

protected:
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    bool event(QEvent *event);

public slots:
    void updateRecentProjects();

signals:
    void deletedProject(const QString &);

private slots:
    void on_projectsFolderButton_clicked();

    void on_videoModeButton_clicked();

    void onProfileTriggered(QAction *action);

    void on_actionAddCustomProfile_triggered();

    void on_actionProfileRemove_triggered();

    void on_startButton_clicked();

    void on_projectNameLineEdit_textChanged(const QString &arg1);

    void on_recentListView_clicked(const QModelIndex &index);

    void on_recentListView_doubleClicked(const QModelIndex &index);

    void on_recentListView_customContextMenuRequested(const QPoint &pos);

    void on_actionRecentRemove_triggered();

private:
    void setColors();
    void setProjectFolderButtonText(const QString &text);

    Ui::NewProjectFolder *ui;
    QActionGroup *m_profileGroup;
    QMenu m_videoModeMenu;
    QMenu *m_customProfileMenu;
    QString m_profile;
    QStandardItemModel m_model;
    QString m_projectName;
    bool m_isOpening;
};

#endif // NEWPROJECTFOLDER_H
