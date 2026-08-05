/*
 * Copyright (c) 2026 Meltytech, LLC
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

#ifndef DOUBLESLIDERSPINNER_H
#define DOUBLESLIDERSPINNER_H

#include <QWidget>

class QDoubleSpinBox;
class QSlider;

class DoubleSliderSpinner : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix)
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix)
    Q_PROPERTY(QString specialValueText READ specialValueText WRITE setSpecialValueText)
    Q_PROPERTY(double defaultValue READ defaultValue WRITE setDefaultValue)
    Q_PROPERTY(bool showResetButton READ showResetButton WRITE setShowResetButton)

public:
    explicit DoubleSliderSpinner(QWidget *parent = nullptr);

    double value() const;
    double minimum() const;
    double maximum() const;
    double singleStep() const;
    int decimals() const;
    QString prefix() const;
    QString suffix() const;
    QString specialValueText() const;
    double defaultValue() const;
    bool showResetButton() const;

public slots:
    void setValue(double value);
    void setMinimum(double value);
    void setMaximum(double value);
    void setRange(double min, double max);
    void setSingleStep(double value);
    void setDecimals(int decimals);
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setSpecialValueText(const QString &text);
    void setDefaultValue(double value);
    void setShowResetButton(bool show);

signals:
    void valueChanged(double value);

private slots:
    void onSliderValueChanged(int value);
    void onSpinValueChanged(double value);
    void onResetButtonClicked();

private:
    int toSliderValue(double value) const;
    double toSpinValue(int value) const;
    void updateScale();
    void updateSliderRange();
    void updateResetButtonState();

    QSlider *m_slider;
    QDoubleSpinBox *m_spinBox;
    class QToolButton *m_resetButton;
    int m_scale;
    double m_defaultValue;
    bool m_showResetButton;
};

#endif // DOUBLESLIDERSPINNER_H