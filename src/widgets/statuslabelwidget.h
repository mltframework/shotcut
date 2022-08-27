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

#ifndef STATUSLABELWIDGET_H
#define STATUSLABELWIDGET_H

#include <QTimer>
#include <QWidget>

class QHBoxLayout;
class QPropertyAnimation;
class QPushButton;

class StatusLabelWidget : public QWidget
{
    Q_OBJECT

public:
    StatusLabelWidget(QWidget *parent = nullptr);
    virtual ~StatusLabelWidget();
    void showText(const QString &text, int timeoutSeconds = -1, QAction *action = nullptr,
                  QPalette::ColorRole role = QPalette::ToolTipBase);

signals:
    void statusCleared();

private:
    void onFadeOutFinished();
    QHBoxLayout *m_layout;
    QPushButton *m_label;
    QPropertyAnimation *m_fadeIn;
    QPropertyAnimation *m_fadeOut;
    QTimer m_timer;
};

#endif // STATUSLABELWIDGET_H
