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
}

class OpenOtherDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenOtherDialog(Mlt::Controller*, QWidget *parent = 0);
    ~OpenOtherDialog();
    
    QString producerName() const;
    QString URL() const;
    Mlt::Properties* mltProperties() const;
    void load(QString& producer, Mlt::Properties& p);

private slots:
    void on_colorButton_clicked();
    
    void on_tempDial_valueChanged(int value);
    void on_tempSpinner_valueChanged(double arg1);
    void on_borderGrowthDial_valueChanged(int value);
    void on_borderGrowthSpinner_valueChanged(double arg1);
    void on_spontGrowthDial_valueChanged(int value);
    void on_spontGrowthSpinner_valueChanged(double arg1);
    
    void on_xratioDial_valueChanged(int value);
    void on_xratioSpinner_valueChanged(double arg1);
    void on_yratioDial_valueChanged(int value);
    void on_yratioSpinner_valueChanged(double arg1);
    
    void on_speed1Dial_valueChanged(int value);
    void on_speed1Spinner_valueChanged(double arg1);
    void on_speed2Dial_valueChanged(int value);
    void on_speed2Spinner_valueChanged(double arg1);
    void on_speed3Dial_valueChanged(int value);
    void on_speed3Spinner_valueChanged(double arg1);
    void on_speed4Dial_valueChanged(int value);
    void on_speed4Spinner_valueChanged(double arg1);
    void on_move1Dial_valueChanged(int value);
    void on_move1Spinner_valueChanged(double arg1);
    void on_move2Dial_valueChanged(int value);
    void on_move2Spinner_valueChanged(double arg1);

    void on_savePresetButton_clicked();
    void on_presetCombo_activated(int index);
    void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void on_deletePresetButton_clicked();
    
private:
    Ui::OpenOtherDialog *ui;
    Mlt::Controller *mlt;
    Mlt::Properties* mltProperties(const QString& producer) const;
    QString URL(const QString& producer) const;
    void saveDefaultPreset(const QString& producer);
    void selectTreeWidget(const QString& s);
    void loadPresets();
};

#endif // OPENOTHERDIALOG_H
