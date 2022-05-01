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

#ifndef ALIGNAUDIODIALOG_H
#define ALIGNAUDIODIALOG_H

#include "models/alignclipsmodel.h"

#include <QDialog>
#include <QUuid>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QListWidget;
class QLineEdit;
class QPushButton;
class QTreeView;

class AlignTableDelegate;
class MultitrackModel;
class LongUiTask;

class AlignAudioDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AlignAudioDialog(QString title, MultitrackModel *model, const QVector<QUuid> &uuids,
                              QWidget *parent = 0);
    virtual ~AlignAudioDialog();

private slots:
    void rebuildClipList();
    void process();
    void apply();
    void processAndApply();
    void updateReferenceProgress(int percent);
    void updateClipProgress(int index, int percent);
    void clipFinished(int index, int offset, double drift, double quality);

private:
    AlignTableDelegate *m_delegate;
    MultitrackModel *m_model;
    AlignClipsModel m_alignClipsModel;
    QVector<QUuid> m_uuids;
    QComboBox *m_trackCombo;
    QTreeView *m_table;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_processAndApplyButton;
    LongUiTask *m_uiTask;
};

#endif // ALIGNAUDIODIALOG_H
