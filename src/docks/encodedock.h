/*
 * Copyright (c) 2012-2014 Meltytech, LLC
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
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

class QTreeWidgetItem;
class QStringList;
namespace Ui {
    class EncodeDock;
}
namespace Mlt {
    class Properties;
}
class AbstractJob;
class MeltJob;

class PresetsProxyModel : public QSortFilterProxyModel
{
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

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
    void onProfileChanged();

private slots:
    void on_presetsTree_clicked(const QModelIndex &index);
    void on_presetsTree_activated(const QModelIndex &index);

    void on_encodeButton_clicked();

    void on_streamButton_clicked();

    void on_addPresetButton_clicked();

    void on_removePresetButton_clicked();

    void onFinished(AbstractJob*, bool isSuccess);

    void on_stopCaptureButton_clicked();

    void on_videoRateControlCombo_activated(int index);

    void on_audioRateControlCombo_activated(int index);

    void on_scanModeCombo_currentIndexChanged(int index);

    void on_presetsSearch_textChanged(const QString &search);

    void on_resetButton_clicked();

private:
    enum {
        RateControlAverage = 0,
        RateControlConstant,
        RateControlQuality
    };
    Ui::EncodeDock *ui;
    Mlt::Properties *m_presets;
    MeltJob* m_immediateJob;
    QString m_extension;
    Mlt::Properties *m_profiles;
    PresetsProxyModel m_presetsModel;

    void loadPresets();
    Mlt::Properties* collectProperties(int realtime);
    void collectProperties(QDomElement& node, int realtime);
    MeltJob* createMeltJob(const QString& target, int realtime, int pass = 0);
    void runMelt(const QString& target, int realtime = -1);
    void enqueueMelt(const QString& target, int realtime);
    void encode(const QString& target);
    void resetOptions();
};

#endif // ENCODEDOCK_H
