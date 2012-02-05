/*
 * Copyright (c) 2012 Meltytech, LLC
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

#ifndef OPENOTHERDIALOG_H
#define OPENOTHERDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>

namespace Ui {
    class OpenOtherDialog;
}
namespace Mlt {
    class Controller;
    class Properties;
    class Producer;
    class Profile;
}

class OpenOtherDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenOtherDialog(Mlt::Controller*, QWidget *parent = 0);
    ~OpenOtherDialog();
    
    Mlt::Producer* producer(Mlt::Profile&) const;
    void load(Mlt::Producer*);

private slots:    
    void on_savePresetButton_clicked();
    void on_presetCombo_activated(int index);
    void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void on_deletePresetButton_clicked();
    
private:
    Ui::OpenOtherDialog *ui;
    Mlt::Controller *mlt;
    QObject* m_current;

    Mlt::Producer* producer(Mlt::Profile&, QObject* widget) const;
    void loadPresets();
    void saveDefaultPreset(QObject* widget);
    void selectTreeWidget(const QString& s);
};

#endif // OPENOTHERDIALOG_H
