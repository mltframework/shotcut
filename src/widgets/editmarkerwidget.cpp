/*
 * Copyright (c) 2021 Meltytech, LLC
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

#include "Logger.h"
#include "qmltypes/qmlapplication.h"
#include "widgets/timespinbox.h"

#include <QColorDialog>
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

EditMarkerWidget::EditMarkerWidget(QWidget *parent, const QString& text, const QColor& color, int start, int end)
    : QWidget(parent)
{
    QGridLayout* grid = new QGridLayout();
    setLayout(grid);

    grid->addWidget(new QLabel(tr("Text")), 0, 0, Qt::AlignRight);
    m_textField = new QLineEdit(text);
    m_textField->setToolTip(tr("Set the text for this marker."));
    grid->addWidget(m_textField, 0, 1);

    grid->addWidget(new QLabel(""), 1, 0, Qt::AlignRight);
    m_colorButton = new QPushButton(tr("Color..."));
    connect(m_colorButton, SIGNAL(clicked()), SLOT(on_colorButton_clicked()));
    m_colorLabel = new QLabel(color.name(QColor::HexRgb));
    m_colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
            .arg((color.value() < 150)? "white":"black")
            .arg(color.name()));
    QHBoxLayout* colorHbox = new QHBoxLayout();
    colorHbox->addWidget(m_colorButton);
    colorHbox->addWidget(m_colorLabel);
    grid->addLayout(colorHbox, 1, 1);

    grid->addWidget(new QLabel(tr("Start")), 2, 0, Qt::AlignRight);
    m_startSpinner = new TimeSpinBox();
    m_startSpinner->setValue(start);
    m_startSpinner->setToolTip(tr("Set the start time for this marker."));
    grid->addWidget(m_startSpinner, 2, 1);

    grid->addWidget(new QLabel(tr("End")), 3, 0, Qt::AlignRight);
    m_endSpinner = new TimeSpinBox();
    m_endSpinner->setValue(end);
    m_endSpinner->setMinimum(start);
    m_endSpinner->setToolTip(tr("Set the end time for this marker."));
    grid->addWidget(m_endSpinner, 3, 1);

    connect(m_startSpinner, SIGNAL(valueChanged(int)), this, SLOT(on_startSpinner_valueChanged(int)));
}

EditMarkerWidget::~EditMarkerWidget()
{
}

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

void EditMarkerWidget::on_colorButton_clicked()
{
    QColor color = QColor(m_colorLabel->text());
    QColorDialog dialog(color);
    dialog.setModal(QmlApplication::dialogModality());
    if (dialog.exec() == QDialog::Accepted) {
        auto newColor = dialog.currentColor();
        m_colorLabel->setText(newColor.name(QColor::HexRgb));
        m_colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
                                      .arg((newColor.value() < 150)? "white":"black")
                                      .arg(newColor.name()));
    }
}

void EditMarkerWidget::on_startSpinner_valueChanged(int value)
{
    m_endSpinner->setMinimum(value);
}
