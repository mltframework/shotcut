/*
 * Copyright (c) 2015-2026 Meltytech, LLC
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

#include "timelineitems.h"

#include "Logger.h"
#include "mltcontroller.h"
#include "models/multitrackmodel.h"
#include "qmltypes/qmlproducer.h"
#include "settings.h"

#include <QLinearGradient>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPointer>
#include <QQmlContext>
#include <QQuickPaintedItem>

class TimelineTransition : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor colorA MEMBER m_colorA NOTIFY propertyChanged)
    Q_PROPERTY(QColor colorB MEMBER m_colorB NOTIFY propertyChanged)

public:
    TimelineTransition()
    {
        setAntialiasing(true);
        connect(this, SIGNAL(propertyChanged()), this, SLOT(update()));
    }

    void paint(QPainter *painter)
    {
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, m_colorA);
        gradient.setColorAt(1, m_colorB);

        QPainterPath path;
        path.moveTo(0, 0);
        path.lineTo(width(), height());
        path.lineTo(width(), 0);
        path.lineTo(0, height());
        painter->fillPath(path, gradient);
        painter->strokePath(path, painter->pen());
    }
signals:
    void propertyChanged();

private:
    QColor m_colorA;
    QColor m_colorB;
};

class TimelinePlayhead : public QQuickPaintedItem
{
    void paint(QPainter *painter)
    {
        QPainterPath path;
        path.moveTo(width(), 0);
        path.lineTo(width() / 2.0, height());
        path.lineTo(0, 0);
        QPalette p;
        painter->fillPath(path, p.color(QPalette::WindowText));
    }
};

class TimelineTriangle : public QQuickPaintedItem
{
public:
    TimelineTriangle() { setAntialiasing(true); }
    void paint(QPainter *painter)
    {
        QPainterPath path;
        path.moveTo(0, 0);
        path.lineTo(width(), 0);
        path.lineTo(0, height());
        painter->fillPath(path, Qt::black);
    }
};

class TimelineWaveform : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QObject *producer MEMBER m_producer)
    Q_PROPERTY(int trackIndex MEMBER m_trackIndex NOTIFY propertyChanged)
    Q_PROPERTY(int clipIndex MEMBER m_clipIndex NOTIFY propertyChanged)
    Q_PROPERTY(QColor fillColor MEMBER m_color NOTIFY propertyChanged)
    Q_PROPERTY(int inPoint MEMBER m_inPoint NOTIFY inPointChanged)
    Q_PROPERTY(int outPoint MEMBER m_outPoint NOTIFY outPointChanged)
    Q_PROPERTY(bool active MEMBER m_isActive NOTIFY propertyChanged)

public:
    TimelineWaveform()
    {
        setAntialiasing(false);
        setOpaquePainting(true);
        if (Settings.timelineFramebufferWaveform())
            setRenderTarget(QQuickPaintedItem::FramebufferObject);
        connect(this, &TimelineWaveform::propertyChanged, this, [this]() { update(); });
    }

    Q_INVOKABLE void connectToModel()
    {
        if (m_model)
            return; // Already connected

        QObject *obj = qmlContext(this)->contextProperty("multitrack").value<QObject *>();
        m_model = qobject_cast<MultitrackModel *>(obj);
        if (m_model) {
            connect(m_model,
                    SIGNAL(
                        dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)),
                    this,
                    SLOT(onDataChanged(const QModelIndex &,
                                       const QModelIndex &,
                                       const QVector<int> &)));
        } else {
            LOG_WARNING() << "TimelineWaveform: Failed to find multitrack model in QML context";
        }
    }

    void paint(QPainter *painter)
    {
        if (!m_isActive)
            return;

        const QVariantList *data = nullptr;
        QmlProducer *qmlProducer = nullptr;
        std::unique_ptr<Mlt::ClipInfo> clipInfo;

        // Get audio levels from producer property if available
        if (m_producer) {
            // Read from producer property (keyframes view)
            qmlProducer = qobject_cast<QmlProducer *>(m_producer);
            if (qmlProducer) {
                qmlProducer->producer().lock();
                data = qmlProducer->audioLevels();
            }
        } else if (m_model && m_trackIndex >= 0 && m_clipIndex >= 0) {
            // Get audio levels pointer directly from the model (timeline view)
            clipInfo = m_model->getClipInfo(m_trackIndex, m_clipIndex);
            if (clipInfo && clipInfo->producer) {
                clipInfo->producer->lock();
                data = m_model->getAudioLevels(m_trackIndex, m_clipIndex);
            }
        }

        if (!data || data->isEmpty()) {
            if (qmlProducer)
                qmlProducer->producer().unlock();
            else if (clipInfo && clipInfo->producer)
                clipInfo->producer->unlock();
            return;
        }

        // In and out points are # frames at current fps,
        // but audio levels are created at 25 fps.
        // Scale in and out point to 25 fps.
        const int inPoint = qRound(m_inPoint / MLT.profile().fps() * 25.0);
        const int outPoint = qRound(m_outPoint / MLT.profile().fps() * 25.0);
        const qreal indicesPrPixel = qreal(outPoint - inPoint) / width();

        //        LOG_DEBUG() << "In/out points" << inPoint << "/" << outPoint;

        QPainterPath path;
        path.moveTo(-1, height());
        int i = 0;
        const int dataSize = data->size();
        for (; i < width(); ++i) {
            int idx = inPoint + int(i * indicesPrPixel);
            if ((idx < 0) || (idx + 1 >= dataSize))
                break;
            // Convert to real only for values actually used
            qreal level = qMax(data->at(idx).toReal(), data->at(idx + 1).toReal()) / 256.0;
            path.lineTo(i, height() - level * height());
        }
        path.lineTo(i, height());
        painter->fillPath(path, m_color.lighter());

        QPen pen(painter->pen());
        pen.setColor(m_color.darker());
        painter->strokePath(path, pen);

        if (qmlProducer)
            qmlProducer->producer().unlock();
        else if (clipInfo && clipInfo->producer)
            clipInfo->producer->unlock();
    }

signals:
    void propertyChanged();
    void inPointChanged();
    void outPointChanged();

private slots:
    void onDataChanged(const QModelIndex &topLeft,
                       const QModelIndex &bottomRight,
                       const QVector<int> &roles)
    {
        // Check if this update is for our clip and includes AudioLevelsRole
        if (m_model && topLeft.isValid()) {
            QModelIndex parentIndex = topLeft.parent();
            if (parentIndex.isValid()) {
                int updateTrackIndex = parentIndex.row();
                int updateClipIndex = topLeft.row();
                if (updateTrackIndex == m_trackIndex && updateClipIndex == m_clipIndex) {
                    if (roles.isEmpty() || roles.contains(MultitrackModel::AudioLevelsRole)) {
                        update();
                    }
                }
            }
        }
    }

private:
    QPointer<QObject> m_producer;
    QPointer<MultitrackModel> m_model;
    int m_trackIndex{-1};
    int m_clipIndex{-1};
    int m_inPoint;
    int m_outPoint;
    QColor m_color;
    bool m_isActive{true};
};

class MarkerStart : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor fillColor MEMBER m_color NOTIFY propertyChanged)

public:
    MarkerStart()
    {
        setAntialiasing(true);
        connect(this, SIGNAL(propertyChanged()), this, SLOT(update()));
    }

    void paint(QPainter *painter)
    {
        QPainterPath path;
        path.moveTo(0, 0);
        path.lineTo(0, 10);
        path.lineTo(7, 17);
        path.lineTo(7, 0);
        path.lineTo(0, 0);
        painter->fillPath(path, m_color);
    }

signals:
    void propertyChanged();

private:
    QColor m_color;
};

class MarkerEnd : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor fillColor MEMBER m_color NOTIFY propertyChanged)

public:
    MarkerEnd()
    {
        setAntialiasing(true);
        connect(this, SIGNAL(propertyChanged()), this, SLOT(update()));
    }

    void paint(QPainter *painter)
    {
        QPainterPath path;
        path.moveTo(0, 17);
        path.lineTo(7, 10);
        path.lineTo(7, 0);
        path.lineTo(0, 0);
        painter->fillPath(path, m_color);
    }

signals:
    void propertyChanged();

private:
    QColor m_color;
};

void registerTimelineItems()
{
    qmlRegisterType<TimelineTransition>("Shotcut.Controls", 1, 0, "TimelineTransition");
    qmlRegisterType<TimelinePlayhead>("Shotcut.Controls", 1, 0, "TimelinePlayhead");
    qmlRegisterType<TimelineTriangle>("Shotcut.Controls", 1, 0, "TimelineTriangle");
    qmlRegisterType<TimelineWaveform>("Shotcut.Controls", 1, 0, "TimelineWaveform");
    qmlRegisterType<MarkerStart>("Shotcut.Controls", 1, 0, "MarkerStart");
    qmlRegisterType<MarkerEnd>("Shotcut.Controls", 1, 0, "MarkerEnd");
}

#include "timelineitems.moc"
