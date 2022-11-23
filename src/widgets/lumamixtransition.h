/*
 * Copyright (c) 2014-2022 Meltytech, LLC
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

#ifndef LUMAMIXTRANSITION_H
#define LUMAMIXTRANSITION_H

#include <QWidget>
#include <MltProducer.h>
#include <MltTransition.h>

namespace Ui {
class LumaMixTransition;
}
class ProducerPreviewWidget;

class LumaMixTransition : public QWidget
{
    Q_OBJECT

public:
    explicit LumaMixTransition(Mlt::Producer &transition, QWidget *parent = 0);
    ~LumaMixTransition();

public slots:
    void onPlaying();

signals:
    void modified();

private slots:
    void on_invertCheckBox_clicked(bool checked);

    void on_softnessSlider_valueChanged(int value);

    void on_crossfadeRadioButton_clicked();

    void on_mixRadioButton_clicked();

    void on_mixSlider_valueChanged(int value);

    void on_lumaCombo_currentRowChanged(int index);

    void startPreview();

    void on_previewCheckBox_clicked(bool checked);

    void on_favoriteButton_clicked();

private:
    Ui::LumaMixTransition *ui;
    Mlt::Producer m_producer;
    int m_maxStockIndex;
    ProducerPreviewWidget *m_preview;
    Mlt::Producer m_previewProducer;

    Mlt::Transition *getTransition(const QString &name);
    void updateCustomLumaLabel(Mlt::Transition &transition);
};

#endif // LUMAMIXTRANSITION_H
