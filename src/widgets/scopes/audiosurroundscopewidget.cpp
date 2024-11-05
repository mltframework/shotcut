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

#include "audiosurroundscopewidget.h"

#include <Logger.h>
#include "settings.h"
#include "widgets/iecscale.h"

#include <QPen>
#include <QPainter>

#include <math.h>

static const int TEXT_MARGIN = 3;

QPointF vectorToPoint(qreal direction, qreal magnitude)
{
    QPointF result;
    result.setX(magnitude * cos(direction * M_PI / 180));
    result.setY(magnitude * sin(direction * M_PI / 180));
    return result;
}

QPointF mapFromCenter(QPointF point, QPointF center)
{
    point.setX(center.x() + point.x());
    point.setY(center.y() - point.y());
    return point;
}

AudioSurroundScopeWidget::AudioSurroundScopeWidget()
    : ScopeWidget("AudioSurround")
    , m_frame()
    , m_renderImg()
    , m_mutex()
    , m_displayImg()
    , m_channelsChanged(true)
    , m_channels(Settings.playerAudioChannels())
{
    LOG_DEBUG() << "begin";

    connect(&Settings, &ShotcutSettings::playerAudioChannelsChanged, this, [&]() {
        m_channelsChanged = true;
        m_channels = Settings.playerAudioChannels();
        requestRefresh();
    });

    LOG_DEBUG() << "end";
}

AudioSurroundScopeWidget::~AudioSurroundScopeWidget()
{
}

QString AudioSurroundScopeWidget::getTitle()
{
    return tr("Audio Surround");
}

void AudioSurroundScopeWidget::refreshScope(const QSize &size, bool full)
{
    Q_UNUSED(full)

    qreal side = qMin(size.width(), size.height());
    QSize squareSize = QSize(side, side);

    if (m_graticuleImg.size() != size || m_channelsChanged) {
        m_graticuleImg = QImage(squareSize, QImage::Format_ARGB32_Premultiplied);
        m_graticuleImg.fill(Qt::transparent);
        QPainter p(&m_graticuleImg);
        p.setRenderHint(QPainter::Antialiasing, true);
        drawGraticule(p, devicePixelRatioF());
        p.end();
    }

    while (m_queue.count() > 0) {
        m_frame = m_queue.pop();
    }

    if (m_frame.is_valid() && m_frame.get_audio_samples() > 0) {
        // Calculate the peak level for each channel
        int channels = m_frame.get_audio_channels();
        int samples = m_frame.get_audio_samples();
        QVector<double> levels;
        const int16_t *audio = m_frame.get_audio();
        for ( int c = 0; c < channels; c++ ) {
            int16_t peak = 0;
            const int16_t *p = audio + c;
            for ( int s = 0; s < samples; s++ ) {
                int16_t sample = abs(*p );
                if (sample > peak) peak = sample;
                p += channels;
            }
            double levelDb = 0.0;
            if (peak == 0) {
                levelDb = -100.0;
            } else {
                levelDb = 20 * log10((double)peak / (double)std::numeric_limits<int16_t>::max());
            }
            levels << IEC_ScaleMax(levelDb, 0);
        }

        // Set up a new image and get ready to paint
        QImage newDisplayImage = m_graticuleImg.copy();
        QPainter p(&newDisplayImage);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen;
        QRectF rect = newDisplayImage.rect();
        QPointF center = rect.center();
        QVector<QPointF> allPoints;
        QRectF insideRect;
        insideRect.setX(fontMetrics().height() + 2 * TEXT_MARGIN);
        insideRect.setY(insideRect.x());
        insideRect.setWidth(rect.width() - 2 * insideRect.x());
        insideRect.setHeight(insideRect.width());
        qreal maxCornerLength = sqrt( 2 * pow(insideRect.width() / 2, 2) );

        // Draw the inside lines from center
        pen.setColor(palette().color(QPalette::Active, QPalette::Highlight));
        pen.setWidth(3);
        p.setPen(pen);
        // Left
        if (channels > 1) {
            qreal magnitude =  levels[0] * maxCornerLength;
            QPointF point = vectorToPoint(135, magnitude);
            point = mapFromCenter(point, center);
            p.drawLine(center, point);
            allPoints << point;
        }
        // Center
        if (channels == 1 || channels > 4) {
            qreal magnitude;
            if (channels == 1)
                magnitude = levels[0];
            else
                magnitude = levels[2];
            magnitude *= insideRect.height() / 2;
            QPointF point = vectorToPoint(90, magnitude);
            point = mapFromCenter(point, center);
            p.drawLine(center, point);
            allPoints << point;
        }
        // Right
        if (channels > 1) {
            qreal magnitude =  levels[1] * maxCornerLength;
            QPointF point = vectorToPoint(45, magnitude);
            point = mapFromCenter(point, center);
            p.drawLine(center, point);
            allPoints << point;
        }
        // Right Surround
        if (channels > 3) {
            qreal magnitude;
            if (channels == 4)
                magnitude = levels[3];
            else
                magnitude = levels[5];
            magnitude *= maxCornerLength;
            QPointF point = vectorToPoint(315, magnitude);
            point = mapFromCenter(point, center);
            p.drawLine(center, point);
            allPoints << point;
        }
        // Left Surround
        if (channels > 3) {
            qreal magnitude;
            if (channels == 4)
                magnitude = levels[2];
            else
                magnitude = levels[4];
            magnitude *= maxCornerLength;
            QPointF point = vectorToPoint(225, magnitude);
            point = mapFromCenter(point, center);
            p.drawLine(center, point);
            allPoints << point;
        }

        // Get HSV highlight values to calculate complimentary colors
        int h, s, v;
        palette().color(QPalette::Active, QPalette::Highlight).getHsv(&h, &s, &v);

        // Draw the outside lines from point-to-point
        // Find a complimentary color
        QColor outline = QColor::fromHsv((h + 120) % 360, s, v);
        pen.setColor(outline);
        p.setPen(pen);
        for (int i = 0; i < allPoints.size() - 1; i++) {
            p.drawLine(allPoints[i], allPoints[i + 1]);
        }
        // Connect the last point to the first point
        if (allPoints.size() > 1) {
            p.drawLine(allPoints[allPoints.size() - 1], allPoints[0]);
        }
        // Connect the L and R if center exists
        if (allPoints.size() > 2 && allPoints.size() != 4) {
            p.drawLine(allPoints[0], allPoints[2]);
        }

        // Draw a circle for the average value
        float x = 0;
        float y = 0;
        for (int i = 0; i < allPoints.size(); i++) {
            x += allPoints[i].x();
            y += allPoints[i].y();
        }
        x /= allPoints.size();
        y /= allPoints.size();
        // Find another complimentary color
        QColor circle = QColor::fromHsv((h + 240) % 360, s, v);
        pen.setColor(circle);
        p.setPen(pen);
        p.drawEllipse(QPointF(x, y), 4, 4);
        p.drawEllipse(QPointF(x, y), 8, 8);

        p.end();

        m_mutex.lock();
        m_displayImg.swap(newDisplayImage);
        m_mutex.unlock();
    } else {
        m_mutex.lock();
        m_displayImg = m_graticuleImg.copy();
        m_mutex.unlock();
    }
}

void AudioSurroundScopeWidget::drawGraticule(QPainter &p, qreal lineWidth)
{
    QPen pen;
    QColor color;
    QRect rect = p.window();
    QPoint labelPosition;
    QString labelText;
    int labelWidth;

    // Left
    labelText = tr("L");
    labelPosition.setX(TEXT_MARGIN);
    labelPosition.setY(fontMetrics().height());
    if (m_channels > 1) {
        color = palette().color(QPalette::Active, QPalette::Text);
    } else {
        color = palette().color(QPalette::Disabled, QPalette::Text);
    }
    pen.setColor(color);
    p.setPen(pen);
    p.drawText(labelPosition, labelText);

    // Center
    labelText = tr("C");
    labelWidth = fontMetrics().horizontalAdvance(labelText);
    labelPosition.setX((rect.width() / 2) - (labelWidth / 2));
    labelPosition.setY(fontMetrics().height());
    if (m_channels == 1 || m_channels == 3 || m_channels == 5 || m_channels == 6) {
        color = palette().color(QPalette::Active, QPalette::Text);
    } else {
        color = palette().color(QPalette::Disabled, QPalette::Text);
    }
    pen.setColor(color);
    p.setPen(pen);
    p.drawText(labelPosition, labelText);

    // Right
    labelText = tr("R");
    labelWidth = fontMetrics().horizontalAdvance(labelText);
    labelPosition.setX(rect.width() - labelWidth - TEXT_MARGIN);
    labelPosition.setY(fontMetrics().height());
    if (m_channels > 1) {
        color = palette().color(QPalette::Active, QPalette::Text);
    } else {
        color = palette().color(QPalette::Disabled, QPalette::Text);
    }
    pen.setColor(color);
    p.setPen(pen);
    p.drawText(labelPosition, labelText);

    // Left Surround
    labelText = tr("LS");
    labelPosition.setX(TEXT_MARGIN);
    labelPosition.setY(rect.height() - TEXT_MARGIN);
    if (m_channels > 3) {
        color = palette().color(QPalette::Active, QPalette::Text);
    } else {
        color = palette().color(QPalette::Disabled, QPalette::Text);
    }
    pen.setColor(color);
    p.setPen(pen);
    p.drawText(labelPosition, labelText);

    // Right Surround
    labelText = tr("RS");
    labelWidth = fontMetrics().horizontalAdvance(labelText);
    labelPosition.setX(rect.width() - labelWidth - TEXT_MARGIN);
    labelPosition.setY(rect.height() - TEXT_MARGIN);
    if (m_channels > 3) {
        color = palette().color(QPalette::Active, QPalette::Text);
    } else {
        color = palette().color(QPalette::Disabled, QPalette::Text);
    }
    pen.setColor(color);
    p.setPen(pen);
    p.drawText(labelPosition, labelText);

    // Draw outside frame
    QRectF insideRect;
    insideRect.setX(fontMetrics().height() + 2 * TEXT_MARGIN);
    insideRect.setY(insideRect.x());
    insideRect.setWidth(rect.width() - 2 * insideRect.x());
    insideRect.setHeight(insideRect.width());
    pen.setColor(palette().color(QPalette::Active, QPalette::Text));
    p.setPen(pen);
    p.drawRect(insideRect);
}

void AudioSurroundScopeWidget::paintEvent(QPaintEvent *)
{
    if (!isVisible())
        return;

    QRect squareRect;
    if (width() > height()) {
        int x = (width() - height()) / 2;
        squareRect = QRect(x, 0, height(), height());
    } else {
        int y = (height() - width()) / 2;
        squareRect = QRect(0, y, width(), width());
    }

    // Create the painter
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // draw the vector image
    m_mutex.lock();
    if (!m_displayImg.isNull()) {
        p.drawImage(squareRect, m_displayImg, m_displayImg.rect());
    } else {
        p.fillRect(squareRect, QBrush(Qt::transparent, Qt::SolidPattern));
    }
    m_mutex.unlock();
}
