/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "panel.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QEvent>

Panel::Panel(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout;
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);

    QFrame *frame = new QFrame;
    frame->setMinimumWidth(250);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setSpacing(0);
    hlayout->setContentsMargins(8, 0, 0, 0);
    frame->setLayout(hlayout);
    m_layout->addWidget(frame);
    m_layout->addStretch();

    m_label = new QLabel(title);
    m_label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    hlayout->addWidget(m_label);

    QPushButton *closeButton = new QPushButton;
    closeButton->setIcon(QIcon::fromTheme("window-close"));
    closeButton->setToolTip(tr("Close"));
    closeButton->setFlat(true);
    hlayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, this, &QWidget::hide);
    connect(closeButton, &QPushButton::clicked, this, &Panel::closed);

    updateStylesheet();
}

void Panel::setWidget(QWidget *widget)
{
    m_layout->insertWidget(1, widget, 1);
}

QWidget *Panel::widget() const
{
    if (m_layout->count() > 2)
        return m_layout->itemAt(1)->widget();
    else
        return 0;
}

bool Panel::event(QEvent *event)
{
    QWidget::event(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        updateStylesheet();
    return false;
}

void Panel::updateStylesheet()
{
    QWidget *frame = m_layout->itemAt(0)->widget();
    const QColor &button = palette().button().color();
    const QColor &buttonText = palette().buttonText().color();
    QColor light = button.darker(105);
    QColor dark = button.darker(115);
    if (palette().base().color().value() < palette().window().color().value()) {
        light = button.lighter(125);
        dark = button.darker(125);
    }
    frame->setStyleSheet(QString("background-color: qlineargradient("
                                 "spread:pad, x1:0, y1:1, x2:0, y2:0,"
                                 "stop:0.4 rgb(%1, %2, %3),"
                                 "stop:0.75 rgb(%4, %5, %6));"
                                 "color: rgb(%7, %8, %9);")
                         .arg(dark.red())
                         .arg(dark.green())
                         .arg(dark.blue())
                         .arg(light.red())
                         .arg(light.green())
                         .arg(light.blue())
                         .arg(buttonText.red())
                         .arg(buttonText.green())
                         .arg(buttonText.blue())
                         );
}
