/*
 * Copyright (c) 2016-2017 Meltytech, LLC
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
class QTimer;

class AudioLoudnessScopeWidget Q_DECL_FINAL : public ScopeWidget
{
    Q_OBJECT

public:
    explicit AudioLoudnessScopeWidget();
    ~AudioLoudnessScopeWidget();
    QString getTitle() Q_DECL_OVERRIDE;
    void setOrientation(Qt::Orientation orientation) Q_DECL_OVERRIDE;
    void setOrientation(Qt::Orientation orientation, bool force);

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;

private slots:
    void resetQview();
    void onResetButtonClicked();
    void onIntegratedToggled(bool checked);
    void onShorttermToggled(bool checked);
    void onMomentaryToggled(bool checked);
    void onRangeToggled(bool checked);
    void onPeakToggled(bool checked);
    void onTruePeakToggled(bool checked);
    void updateMeters(void);

private:
    // Functions run in scope thread.
    void refreshScope(const QSize &size, bool full) Q_DECL_OVERRIDE;

    // Members accessed by scope thread.
    Mlt::Filter *m_loudnessFilter;
    double m_peak;
    double m_true_peak;
    bool m_newData;

    // Members accessed by GUI thread.
    Qt::Orientation m_orientation;
    QQuickWidget *m_qview;
    QLabel *m_timeLabel;
    QTimer *m_timer;
};

#endif // AUDIOLOUDNESSSCOPEWIDGET_H
