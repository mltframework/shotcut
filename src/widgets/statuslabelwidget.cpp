/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "statuslabelwidget.h"

#include "settings.h"

#include <QAction>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QPushButton>

static const int STATUS_ANIMATION_MS = 350;

StatusLabelWidget::StatusLabelWidget(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QHBoxLayout;
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_label = new QPushButton();
    m_label->setFlat(true);
    m_label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_label->setAutoFillBackground(true);
    m_layout->addWidget(m_label);
    m_layout->addStretch(1);
    if (Settings.drawMethod() != Qt::AA_UseOpenGLES) {
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
        m_label->setGraphicsEffect(effect);
        m_fadeIn = new QPropertyAnimation(effect, "opacity", this);
        m_fadeIn->setDuration(STATUS_ANIMATION_MS);
        m_fadeIn->setStartValue(0);
        m_fadeIn->setEndValue(1);
        m_fadeIn->setEasingCurve(QEasingCurve::InBack);
        m_fadeOut = new QPropertyAnimation(effect, "opacity", this);
        m_fadeOut->setDuration(STATUS_ANIMATION_MS);
        m_fadeOut->setStartValue(0);
        m_fadeOut->setEndValue(0);
        m_fadeOut->setEasingCurve(QEasingCurve::OutBack);
        m_timer.setSingleShot(true);
        connect(&m_timer, SIGNAL(timeout()), m_fadeOut, SLOT(start()));
        connect(m_fadeOut, &QPropertyAnimation::finished, this, &StatusLabelWidget::onFadeOutFinished);
        m_fadeOut->start();
    } else {
        connect(&m_timer, &QTimer::timeout, this, &StatusLabelWidget::onFadeOutFinished);
    }
    setLayout(m_layout);
}

StatusLabelWidget::~StatusLabelWidget()
{
}

void StatusLabelWidget::showText(const QString &text, int timeoutSeconds, QAction *action,
                                 QPalette::ColorRole role)
{
    QString s = QString("  %1  ").arg(
                    m_label->fontMetrics().elidedText(text, Qt::ElideRight, m_layout->maximumSize().width() - 30));
    m_label->setText(s);
    m_label->setToolTip(text);
    auto palette = QApplication::palette();
    if (role == QPalette::ToolTipBase) {
        palette.setColor(QPalette::Button, palette.color(role));
        palette.setColor(QPalette::ButtonText, palette.color(QPalette::ToolTipText));
    } else {
        palette.setColor(QPalette::Button, palette.color(role));
        palette.setColor(QPalette::ButtonText, palette.color(QPalette::WindowText));
    }
    m_label->setPalette(palette);

    if (action)
        connect(m_label, &QPushButton::clicked, action, &QAction::triggered);
    else
        disconnect(m_label, &QPushButton::clicked, 0, 0);

    if (Settings.drawMethod() != Qt::AA_UseOpenGLES) {
        // Cancel the fade out.
        if (m_fadeOut->state() == QAbstractAnimation::Running) {
            m_fadeOut->stop();
        }
        if (text.isEmpty()) {
            // Make it transparent.
            m_timer.stop();
            m_fadeOut->setStartValue(0);
            m_fadeOut->start();
        } else {
            // Reset the fade out animation.
            m_fadeOut->setStartValue(1);

            // Fade in.
            if (m_fadeIn->state() != QAbstractAnimation::Running && !m_timer.isActive()) {
                m_fadeIn->start();
                if (timeoutSeconds > 0)
                    m_timer.start(timeoutSeconds * 1000);
            }
        }
    } else { // DirectX
        if (text.isEmpty()) {
            m_label->hide();
        } else {
            m_label->show();
            if (timeoutSeconds > 0)
                m_timer.start(timeoutSeconds * 1000);
        }
    }
}

void StatusLabelWidget::onFadeOutFinished()
{
    m_label->disconnect(SIGNAL(clicked(bool)));
    m_label->setToolTip(QString());
    emit statusCleared();
}
