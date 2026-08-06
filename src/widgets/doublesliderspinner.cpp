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

#include "doublesliderspinner.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>

#include <cmath>

DoubleSliderSpinner::DoubleSliderSpinner(QWidget *parent)
    : QWidget(parent)
    , m_slider(new QSlider(Qt::Horizontal, this))
    , m_spinBox(new QDoubleSpinBox(this))
    , m_resetButton(new QToolButton(this))
    , m_scale(1)
    , m_defaultValue(0.0)
    , m_showResetButton(false)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(m_slider, 1);
    layout->addWidget(m_spinBox);
    layout->addWidget(m_resetButton);

    m_spinBox->setKeyboardTracking(false);
    m_spinBox->setDecimals(0);
    m_spinBox->setRange(0.0, 100.0);
    m_spinBox->setSingleStep(1.0);
    m_spinBox->setValue(0.0);
    m_spinBox->setAlignment(Qt::AlignRight);

    m_resetButton->setAutoRaise(true);
    m_resetButton->setIcon(QIcon::fromTheme("edit-undo"));
    m_resetButton->setText(tr("Undo"));
    m_resetButton->setToolTip(tr("Restore default"));
    m_resetButton->setVisible(false);

    updateScale();
    updateSliderRange();
    m_slider->setValue(toSliderValue(m_spinBox->value()));
    updateResetButtonState();

    connect(m_slider, &QSlider::valueChanged, this, &DoubleSliderSpinner::onSliderValueChanged);
    connect(m_spinBox,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &DoubleSliderSpinner::onSpinValueChanged);
    connect(m_resetButton, &QToolButton::clicked, this, &DoubleSliderSpinner::onResetButtonClicked);
}

double DoubleSliderSpinner::value() const
{
    return m_spinBox->value();
}

double DoubleSliderSpinner::minimum() const
{
    return m_spinBox->minimum();
}

double DoubleSliderSpinner::maximum() const
{
    return m_spinBox->maximum();
}

double DoubleSliderSpinner::singleStep() const
{
    return m_spinBox->singleStep();
}

int DoubleSliderSpinner::decimals() const
{
    return m_spinBox->decimals();
}

QString DoubleSliderSpinner::prefix() const
{
    return m_spinBox->prefix();
}

QString DoubleSliderSpinner::suffix() const
{
    return m_spinBox->suffix();
}

QString DoubleSliderSpinner::specialValueText() const
{
    return m_spinBox->specialValueText();
}

double DoubleSliderSpinner::defaultValue() const
{
    return m_defaultValue;
}

bool DoubleSliderSpinner::showResetButton() const
{
    return m_showResetButton;
}

void DoubleSliderSpinner::setValue(double value)
{
    const QSignalBlocker blockSlider(m_slider);
    const QSignalBlocker blockSpin(m_spinBox);
    m_spinBox->setValue(value);
    m_slider->setValue(toSliderValue(m_spinBox->value()));
    updateResetButtonState();
}

void DoubleSliderSpinner::setMinimum(double value)
{
    setRange(value, maximum());
}

void DoubleSliderSpinner::setMaximum(double value)
{
    setRange(minimum(), value);
}

void DoubleSliderSpinner::setRange(double min, double max)
{
    const QSignalBlocker blockSlider(m_slider);
    const QSignalBlocker blockSpin(m_spinBox);
    m_spinBox->setRange(min, max);
    updateSliderRange();
    m_slider->setValue(toSliderValue(m_spinBox->value()));
    updateResetButtonState();
}

void DoubleSliderSpinner::setSingleStep(double value)
{
    const QSignalBlocker blockSlider(m_slider);
    const QSignalBlocker blockSpin(m_spinBox);
    m_spinBox->setSingleStep(value);
    updateSliderRange();
    m_slider->setValue(toSliderValue(m_spinBox->value()));
}

void DoubleSliderSpinner::setDecimals(int decimals)
{
    const QSignalBlocker blockSlider(m_slider);
    const QSignalBlocker blockSpin(m_spinBox);
    m_spinBox->setDecimals(decimals);
    updateScale();
    updateSliderRange();
    m_slider->setValue(toSliderValue(m_spinBox->value()));
}

void DoubleSliderSpinner::setPrefix(const QString &prefix)
{
    m_spinBox->setPrefix(prefix);
}

void DoubleSliderSpinner::setSuffix(const QString &suffix)
{
    m_spinBox->setSuffix(suffix);
}

void DoubleSliderSpinner::setSpecialValueText(const QString &text)
{
    m_spinBox->setSpecialValueText(text);
}

void DoubleSliderSpinner::setDefaultValue(double value)
{
    m_defaultValue = value;
    updateResetButtonState();
}

void DoubleSliderSpinner::setShowResetButton(bool show)
{
    m_showResetButton = show;
    m_resetButton->setVisible(show);
    updateResetButtonState();
}

void DoubleSliderSpinner::onSliderValueChanged(int value)
{
    const double spinValue = toSpinValue(value);
    const QSignalBlocker blockSpin(m_spinBox);
    m_spinBox->setValue(spinValue);
    updateResetButtonState();
    emit valueChanged(m_spinBox->value());
}

void DoubleSliderSpinner::onSpinValueChanged(double value)
{
    const QSignalBlocker blockSlider(m_slider);
    m_slider->setValue(toSliderValue(value));
    updateResetButtonState();
    emit valueChanged(value);
}

void DoubleSliderSpinner::onResetButtonClicked()
{
    if (!m_showResetButton)
        return;
    m_spinBox->setValue(m_defaultValue);
}

int DoubleSliderSpinner::toSliderValue(double value) const
{
    return qRound64(value * m_scale);
}

double DoubleSliderSpinner::toSpinValue(int value) const
{
    return (double) value / m_scale;
}

void DoubleSliderSpinner::updateScale()
{
    m_scale = qRound(pow(10.0, m_spinBox->decimals()));
    if (m_scale < 1)
        m_scale = 1;
}

void DoubleSliderSpinner::updateSliderRange()
{
    const int min = toSliderValue(m_spinBox->minimum());
    const int max = toSliderValue(m_spinBox->maximum());
    const int step = qMax(1, toSliderValue(m_spinBox->singleStep()));

    m_slider->setRange(min, max);
    m_slider->setSingleStep(step);
    m_slider->setPageStep(step * 10);
}

void DoubleSliderSpinner::updateResetButtonState()
{
    if (!m_showResetButton)
        return;
    m_resetButton->setEnabled(!qFuzzyCompare(m_spinBox->value() + 1.0, m_defaultValue + 1.0));
}
