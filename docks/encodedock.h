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

#ifndef ENCODEDOCK_H
#define ENCODEDOCK_H

#include <QDockWidget>

class QTreeWidgetItem;
class QStringList;
namespace Ui {
    class EncodeDock;
}
namespace Mlt {
    class Properties;
}

class EncodeDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit EncodeDock(QWidget *parent = 0);
    ~EncodeDock();

private slots:
    void on_presetsTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void on_encodeButton_clicked();

    void on_reloadSignalButton_clicked();

    void on_streamButton_clicked();

    void on_addPresetButton_clicked();

    void on_removePresetButton_clicked();

private:
    Ui::EncodeDock *ui;
    Mlt::Properties *m_presets;

    void loadPresets();
    QStringList* collectProperties();
    void runMelt(QString& target, int real_time = -1);
};

#endif // ENCODEDOCK_H
