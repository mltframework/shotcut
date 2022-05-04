/*
 * Copyright (c) 2017-2021 Meltytech, LLC
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

#ifndef TRANSCODEDIALOG_H
#define TRANSCODEDIALOG_H

#include <QDialog>

namespace Ui {
class TranscodeDialog;
}

class TranscodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TranscodeDialog(const QString &message, bool isProgressive, QWidget *parent = nullptr);
    ~TranscodeDialog();
    int format() const
    {
        return m_format;
    }
    void showCheckBox();
    bool isCheckBoxChecked() const
    {
        return m_isChecked;
    }
    bool deinterlace() const;
    bool fpsOverride() const;
    double fps() const;
    QString frc() const;
    bool get709Convert();
    void set709Convert(bool enable);
    void showSubClipCheckBox();
    bool isSubClip() const;
    void setSubClipChecked(bool checked);

private slots:
    void on_horizontalSlider_valueChanged(int position);

    void on_checkBox_clicked(bool checked);

    void on_advancedCheckBox_clicked(bool checked);

private:
    Ui::TranscodeDialog *ui;
    int m_format;
    bool m_isChecked;
    bool m_isProgressive;
};

#endif // TRANSCODEDIALOG_H
