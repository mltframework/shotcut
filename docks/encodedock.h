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
#include <QDomElement>

class QTreeWidgetItem;
class QStringList;
namespace Ui {
    class EncodeDock;
}
namespace Mlt {
    class Properties;
}
class MeltJob;

class EncodeDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit EncodeDock(QWidget *parent = 0);
    ~EncodeDock();

signals:
    void captureStateChanged(bool);

public slots:
    void onProducerOpened();

private slots:
    void on_presetsTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void on_encodeButton_clicked();

    void on_reloadSignalButton_clicked();

    void on_streamButton_clicked();

    void on_addPresetButton_clicked();

    void on_removePresetButton_clicked();
    void onFinished(MeltJob*, bool isSuccess);

    void on_stopCaptureButton_clicked();

private:
    Ui::EncodeDock *ui;
    Mlt::Properties *m_presets;
    MeltJob* m_immediateJob;
    QString m_extension;
    Mlt::Properties *m_profiles;

    void loadPresets();
    Mlt::Properties* collectProperties(int realtime);
    void collectProperties(QDomElement& node, int realtime);
    MeltJob* createMeltJob(const QString& target, int realtime = -1, int pass = 0);
    void runMelt(const QString& target, int realtime = -1);
    void enqueueMelt(const QString& target, int realtime = -1);
    void encode(const QString& target);
};

#endif // ENCODEDOCK_H
