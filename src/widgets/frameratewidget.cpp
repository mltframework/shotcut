/*
 * Copyright (c) 2020 Meltytech, LLC
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

#include "widgets/frameratewidget.h"

#include "util.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>

FrameRateWidget::FrameRateWidget(QWidget *parent)
    : QWidget(parent)
    , m_fps(0.0)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_fpsSpinner = new QDoubleSpinBox();
    m_fpsSpinner->setDecimals(6);
    m_fpsSpinner->setMinimum(1.0);
    m_fpsSpinner->setMaximum(1000);
    m_fpsSpinner->setValue(25.0);
    connect(m_fpsSpinner, SIGNAL(editingFinished()), this, SLOT(on_fpsSpinner_editingFinished()));
    layout->addWidget(m_fpsSpinner);

    m_fpsComboBox = new QComboBox();
    m_fpsComboBox->setMaximumSize(20, 16777215);
    m_fpsComboBox->addItem("");
    m_fpsComboBox->addItem("23.976024");
    m_fpsComboBox->addItem("24");
    m_fpsComboBox->addItem("25");
    m_fpsComboBox->addItem("29.970030");
    m_fpsComboBox->addItem("30");
    m_fpsComboBox->addItem("48");
    m_fpsComboBox->addItem("50");
    m_fpsComboBox->addItem("59.940060");
    m_fpsComboBox->addItem("60");
    connect(m_fpsComboBox, SIGNAL(currentTextChanged(const QString &)), this,
            SLOT(on_fpsComboBox_activated(const QString &)));
    layout->addWidget(m_fpsComboBox);
}

double FrameRateWidget::fps()
{
    return m_fpsSpinner->value();
}

void FrameRateWidget::setFps(double fps)
{
    m_fpsSpinner->setValue(fps);
}

void FrameRateWidget::on_fpsSpinner_editingFinished()
{
    if (m_fpsSpinner->value() != m_fps) {
        const QString caption(tr("Convert Frames/sec"));
        if (m_fpsSpinner->value() == 23.98 || m_fpsSpinner->value() == 23.976) {
            Util::showFrameRateDialog(caption, 24000, m_fpsSpinner, this);
        } else if (m_fpsSpinner->value() == 29.97) {
            Util::showFrameRateDialog(caption, 30000, m_fpsSpinner, this);
        } else if (m_fpsSpinner->value() == 47.95) {
            Util::showFrameRateDialog(caption, 48000, m_fpsSpinner, this);
        } else if (m_fpsSpinner->value() == 59.94) {
            Util::showFrameRateDialog(caption, 60000, m_fpsSpinner, this);
        }
        m_fps = m_fpsSpinner->value();
        emit fpsChanged(m_fps);
    }
}

void FrameRateWidget::on_fpsComboBox_activated(const QString &arg1)
{
    if (!arg1.isEmpty())
        m_fpsSpinner->setValue(arg1.toDouble());
}
