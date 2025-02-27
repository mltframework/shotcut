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

#ifndef MLTCLIPPRODUCERWIDGET_H
#define MLTCLIPPRODUCERWIDGET_H

#include "abstractproducerwidget.h"

#include <MltService.h>
#include <QWidget>

class QLabel;

class MltClipProducerWidget : public QWidget, public AbstractProducerWidget
{
    Q_OBJECT

public:
    explicit MltClipProducerWidget(QWidget *parent = 0);
    ~MltClipProducerWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &);
    void setProducer(Mlt::Producer *);

private:
    QLabel *m_nameLabel;
    QLabel *m_resolutionLabel;
    QLabel *m_aspectRatioLabel;
    QLabel *m_frameRateLabel;
    QLabel *m_scanModeLabel;
    QLabel *m_colorspaceLabel;
    QLabel *m_durationLabel;
    QLabel *m_errorIcon;
    QLabel *m_errorText;
};

#endif //MLTCLIPPRODUCERWIDGET_H
