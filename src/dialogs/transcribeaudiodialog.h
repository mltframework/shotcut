/*
 * Copyright (c) 2024 Meltytech, LLC
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

#ifndef TRANSCRIBEAUDIODIALOG_H
#define TRANSCRIBEAUDIODIALOG_H

#include <QDialog>

class QAbstractButton;
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QListWidget;
class QSpinBox;

class TranscribeAudioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TranscribeAudioDialog(const QString &trackName, QWidget *parent);
    QString name();
    QString language();
    QList<int> tracks();
    bool translate();
    int maxLineLength();
    bool includeNonspoken();

private slots:
    void clicked(QAbstractButton *button);

private:
    void updateWhisperStatus();
    QLineEdit *m_name;
    QComboBox *m_lang;
    QCheckBox *m_translate;
    QSpinBox *m_maxLength;
    QCheckBox *m_nonspoken;
    QListWidget *m_trackList;
    QWidget *m_configWidget;
    QLineEdit *m_exeLabel;
    QLineEdit *m_modelLabel;
    QDialogButtonBox *m_buttonBox;
};

#endif // TRANSCRIBEAUDIODIALOG_H
