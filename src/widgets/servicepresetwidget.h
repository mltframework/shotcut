/*
 * Copyright (c) 2012-2025 Meltytech, LLC
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

#ifndef SERVICEPRESETWIDGET_H
#define SERVICEPRESETWIDGET_H

#include <MltProperties.h>
#include <QWidget>

namespace Ui {
class ServicePresetWidget;
}

class ServicePresetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ServicePresetWidget(QWidget *parent = 0);
    ~ServicePresetWidget();

    void loadPresets();
    void saveDefaultPreset(const Mlt::Properties &);
    void savePreset(const Mlt::Properties &);
    void savePreset(const Mlt::Properties &properties, QString name);
    void setPreset(const QString &name);

signals:
    void selected(void *properties);
    void saveClicked();

private slots:
    void on_presetCombo_activated(int index);
    void on_savePresetButton_clicked();
    void on_deletePresetButton_clicked();

private:
    Ui::ServicePresetWidget *ui;
    QString m_widgetName;
};

#endif // SERVICEPRESETWIDGET_H
