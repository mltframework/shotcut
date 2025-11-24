/*
 * Copyright (c) 2021-2025 Meltytech, LLC
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

#include "editmarkerwidget.h"

#include "mltcontroller.h"
#include "qmltypes/colordialog.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include "util.h"
#include "widgets/timespinbox.h"

#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>

EditMarkerWidget::EditMarkerWidget(
    QWidget *parent, const QString &text, const QColor &color, int start, int end, int maxEnd)
    : QWidget(parent)
{
    QGridLayout *grid = new QGridLayout();
    setLayout(grid);
    grid->setColumnMinimumWidth(0, 125);
    grid->setColumnMinimumWidth(1, 125);

    m_textField = new QLineEdit(text);
    connect(m_textField, SIGNAL(editingFinished()), SIGNAL(valuesChanged()));
    m_textField->setToolTip(tr("Set the name for this marker."));
    grid->addWidget(m_textField, 0, 0, 1, 2);

    m_colorButton = new QPushButton(tr("Color..."));
    connect(m_colorButton, SIGNAL(clicked()), SLOT(on_colorButton_clicked()));
    grid->addWidget(m_colorButton, 1, 0, Qt::AlignRight);
    m_colorLabel = new QLabel(color.name(QColor::HexRgb));
    m_colorLabel->setStyleSheet(
        QStringLiteral("color: %1; background-color: %2").arg(Util::textColor(color), color.name()));
    grid->addWidget(m_colorLabel, 1, 1);

    grid->addWidget(new QLabel(tr("Start")), 2, 0, Qt::AlignRight);
    m_startSpinner = new TimeSpinBox();
    m_startSpinner->setMinimum(0);
    m_startSpinner->setMaximum(end);
    m_startSpinner->setValue(start);
    m_startSpinner->setToolTip(tr("Set the start time for this marker."));
    connect(m_startSpinner,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(on_startSpinner_valueChanged(int)));
    grid->addWidget(m_startSpinner, 2, 1);

    grid->addWidget(new QLabel(tr("End")), 3, 0, Qt::AlignRight);
    m_endSpinner = new TimeSpinBox();
    m_endSpinner->setMinimum(start);
    m_endSpinner->setMaximum(maxEnd);
    m_endSpinner->setValue(end);
    m_endSpinner->setToolTip(tr("Set the end time for this marker."));
    connect(m_endSpinner, SIGNAL(valueChanged(int)), this, SLOT(on_endSpinner_valueChanged(int)));
    grid->addWidget(m_endSpinner, 3, 1);

    grid->addWidget(new QLabel(tr("Duration:")), 4, 0, Qt::AlignRight);
    m_durationLabel = new QLabel();
    updateDuration();
    grid->addWidget(m_durationLabel, 4, 1);
}

EditMarkerWidget::~EditMarkerWidget() {}

QString EditMarkerWidget::getText()
{
    return m_textField->text();
}

QColor EditMarkerWidget::getColor()
{
    return QColor(m_colorLabel->text());
}

int EditMarkerWidget::getStart()
{
    return m_startSpinner->value();
}

int EditMarkerWidget::getEnd()
{
    return m_endSpinner->value();
}

void EditMarkerWidget::setValues(
    const QString &text, const QColor &color, int start, int end, int maxEnd)
{
    QSignalBlocker textBlocker(m_textField);
    QSignalBlocker colorBlocker(m_colorLabel);
    QSignalBlocker startBlocker(m_startSpinner);
    QSignalBlocker endBlocker(m_endSpinner);
    m_textField->setText(text);
    m_colorLabel->setText(color.name(QColor::HexRgb));
    m_colorLabel->setStyleSheet(
        QStringLiteral("color: %1; background-color: %2").arg(Util::textColor(color), color.name()));
    m_startSpinner->setMinimum(0);
    m_startSpinner->setMaximum(end);
    m_startSpinner->setValue(start);
    m_endSpinner->setMinimum(start);
    m_endSpinner->setMaximum(maxEnd);
    m_endSpinner->setValue(end);
    updateDuration();
    emit valuesChanged();
}

void EditMarkerWidget::on_colorButton_clicked()
{
    QColor color = QColor(m_colorLabel->text());
    auto newColor = ColorDialog::getColor(color, this, QString(), false);

    if (newColor.isValid()) {
        m_colorLabel->setText(newColor.name(QColor::HexRgb));
        m_colorLabel->setStyleSheet(QStringLiteral("color: %1; background-color: %2")
                                        .arg(Util::textColor(newColor), newColor.name()));
    }

    emit valuesChanged();
}

void EditMarkerWidget::on_startSpinner_valueChanged(int value)
{
    m_endSpinner->setMinimum(value);
    updateDuration();
    emit valuesChanged();
}

void EditMarkerWidget::on_endSpinner_valueChanged(int value)
{
    m_startSpinner->setMaximum(value);
    updateDuration();
    emit valuesChanged();
}

void EditMarkerWidget::updateDuration()
{
    if (MLT.producer()) {
        int duration = m_endSpinner->value() - m_startSpinner->value() + 1;
        m_durationLabel->setText(MLT.producer()->frames_to_time(duration, Settings.timeFormat()));
    } else {
        m_durationLabel->setText("--:--:--:--");
    }
}
