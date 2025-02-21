/*
 * Copyright (c) 2024 Meltytech, LLC
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

#ifndef AUDIOVECTORSCOPEWIDGET_H
#define AUDIOVECTORSCOPEWIDGET_H

#include "scopewidget.h"

#include <QImage>
#include <QMutex>

class QComboBox;
class QLabel;

class AudioVectorScopeWidget Q_DECL_FINAL : public ScopeWidget
{
    Q_OBJECT

public:
    explicit AudioVectorScopeWidget();
    ~AudioVectorScopeWidget();
    QString getTitle() Q_DECL_OVERRIDE;

private:
    // Functions run in scope thread.
    void refreshScope(const QSize &size, bool full) Q_DECL_OVERRIDE;

    // Functions run in GUI thread.
    void setComboBoxOptions();
    Q_INVOKABLE void onNewDisplayImage();

    // Members accessed only in scope thread (no thread protection).
    QImage m_renderImg;
    SharedFrame m_frame;

    // Members accessed only in GUI thread (no thread protection).
    QComboBox *m_c1Combo;
    QComboBox *m_c2Combo;
    QLabel *m_imgLabel;

    // Members accessed in multiple threads (mutex protected).
    QMutex m_mutex;
    QImage m_displayImg;
    int m_c1Index;
    int m_c2Index;
};

#endif // AUDIOVECTORSCOPEWIDGET_H
