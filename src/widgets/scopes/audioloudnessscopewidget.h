/*
 * Copyright (c) 2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

#ifndef AUDIOLOUDNESSSCOPEWIDGET_H
#define AUDIOLOUDNESSSCOPEWIDGET_H

#include "scopewidget.h"
#include <QMutex>
#include <QImage>
#include <QVector>
#include <MltFilter.h>

class QQuickWidget;
class QLabel;

class AudioLoudnessScopeWidget Q_DECL_FINAL : public ScopeWidget
{
    Q_OBJECT
    
public:
    explicit AudioLoudnessScopeWidget();
    ~AudioLoudnessScopeWidget();
    QString getTitle();
    void setOrientation(Qt::Orientation orientation) Q_DECL_OVERRIDE;

protected:
    bool event(QEvent *event);

private slots:
    void resetQview();
    void onResetButtonClicked();

private:
    // Functions run in scope thread.
    void refreshScope(const QSize& size, bool full) Q_DECL_OVERRIDE;

    // Members accessed by scope thread.
    Mlt::Filter* m_loudnessFilter;
    unsigned int m_msElapsed;

    // Members accessed by GUI thread.
    Qt::Orientation m_orientation;
    QQuickWidget* m_qview;
    QLabel* m_timeLabel;
};

#endif // AUDIOLOUDNESSSCOPEWIDGET_H
