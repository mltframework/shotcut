/*
 * Copyright (c) 2023 Meltytech, LLC
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

#ifndef METALVIDEOWIDGET_H
#define METALVIDEOWIDGET_H

#include "videowidget.h"

class MetalVideoRenderer;

class MetalVideoWidget : public Mlt::VideoWidget
{
    Q_OBJECT
public:
    explicit MetalVideoWidget(QObject *parent);
    virtual ~MetalVideoWidget();

public slots:
    virtual void initialize();
    virtual void renderVideo();

private:
    std::unique_ptr<MetalVideoRenderer> m_renderer;
};

#endif // METALVIDEOWIDGET_H
