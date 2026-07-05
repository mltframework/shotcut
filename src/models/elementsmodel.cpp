/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "elementsmodel.h"

#include "Logger.h"
#include "database.h"
#include "mltcontroller.h"
#include "models/playlistmodel.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPolygon>
#include <QRunnable>
#include <QThreadPool>

static const QStringList kElementExtensions{"*.tgs", "*.flac", "*.json", "*.rawr", "*.lot"};

// ---------------------------------------------------------------------------
// Background thumbnail task
// ---------------------------------------------------------------------------

class ElementsThumbnailTask : public QRunnable
{
    ElementsModel *m_model;
    QString m_filePath;
    QPersistentModelIndex m_index;

public:
    ElementsThumbnailTask(ElementsModel *model, const QString &filePath, const QModelIndex &index)
        : QRunnable()
        , m_model(model)
        , m_filePath(filePath)
        , m_index(index)
    {
        setAutoDelete(true);
    }

    static QString cacheKey(const QString &filePath)
    {
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(filePath.toUtf8());
        return hash.result().toHex();
    }

    void run() override
    {
        LOG_DEBUG() << m_filePath;
        QImage image;
        if (QFileInfo(m_filePath).suffix().compare("flac", Qt::CaseInsensitive) == 0) {
            image = generateAudioThumbnail();
        } else {
            static Mlt::Profile profile{"atsc_720p_60"};
            Mlt::Producer producer(profile, "abnormal", m_filePath.toUtf8().constData());
            if (producer.is_valid()) {
                Mlt::Filter scaler(profile, "swscale");
                Mlt::Filter padder(profile, "resize");
                Mlt::Filter converter(profile, "avcolor_space");
                producer.attach(scaler);
                producer.attach(padder);
                producer.attach(converter);
                const auto width = PlaylistModel::THUMBNAIL_WIDTH * 2;
                const auto height = PlaylistModel::THUMBNAIL_HEIGHT * 2;
                image = MLT.image(producer, 0, width, height);
            }
        }
        if (!image.isNull()) {
            m_model->updateThumbnail(m_filePath, image, m_index);
        }
    }

private:
    QImage generateAudioThumbnail() const
    {
        const int width = PlaylistModel::THUMBNAIL_WIDTH * 2;
        const int height = PlaylistModel::THUMBNAIL_HEIGHT * 2;
        QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);

        // Dark background
        p.setBrush(QColor(0x1a, 0x1a, 0x1a));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(image.rect(), 4, 4);

        // Deterministic waveform bars seeded from the path hash
        const auto key = cacheKey(m_filePath);
        quint32 seed = 0;
        for (int i = 0; i < 8 && i < key.size(); ++i)
            seed = seed * 256u + static_cast<quint8>(key[i].toLatin1());

        const int numBars = 18;
        const int margin = width / 10;
        const int totalBarArea = width - 2 * margin;
        const int barW = qMax(2, totalBarArea / (numBars * 2));
        const int barSpacing = totalBarArea / numBars;
        const int maxBarH = height * 7 / 10;
        const QColor barColor(0x00, 0xcc, 0x55);

        p.setBrush(barColor);
        for (int i = 0; i < numBars; ++i) {
            // LCG pseudo-random
            seed = seed * 1664525u + 1013904223u;
            const float t = static_cast<float>(seed & 0xFFFF) / 0xFFFF;
            // Envelope: taller in the middle
            const float env = 1.0f - 0.4f * qAbs(2.0f * i / (numBars - 1) - 1.0f);
            const int barH = qMax(4, static_cast<int>((0.2f + 0.8f * t) * env * maxBarH));
            const int bx = margin + i * barSpacing;
            const int by = (height - barH) / 2;
            p.drawRect(bx, by, barW, barH);
        }

        // Play button (semi-transparent white triangle)
        const int triR = height / 4;
        const int cx = width / 2;
        const int cy = height / 2;
        QPolygon tri;
        tri << QPoint(cx - triR * 7 / 10, cy - triR) << QPoint(cx - triR * 7 / 10, cy + triR)
            << QPoint(cx + triR, cy);
        p.setBrush(QColor(255, 255, 255, 160));
        p.drawPolygon(tri);

        p.end();
        return image;
    }
};

// ---------------------------------------------------------------------------
// ElementsModel
// ---------------------------------------------------------------------------

ElementsModel::ElementsModel(const QDir &dir, QObject *parent)
    : QAbstractListModel(parent)
    , m_dir(dir)
{
    m_files = dir.entryInfoList(kElementExtensions,
                                QDir::Files | QDir::Readable,
                                QDir::Name | QDir::IgnoreCase);
}

int ElementsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_files.count();
}

QVariant ElementsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_files.count())
        return {};
    const auto &info = m_files.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return info.completeBaseName();

    case FilePathRole:
        return info.filePath();

    case HasAnimationRole: {
        const QString webpPath = info.dir().filePath(info.completeBaseName()
                                                     + QStringLiteral(".webp"));
        return QFile::exists(webpPath);
    }

    case ThumbnailRole: {
        if (m_overrideThumbnails.contains(index.row()))
            return m_overrideThumbnails[index.row()];
        const auto key = ElementsThumbnailTask::cacheKey(info.filePath());
        auto image = DB.getThumbnail(key);
        if (image.isNull()) {
            QThreadPool::globalInstance()->start(
                new ElementsThumbnailTask(const_cast<ElementsModel *>(this),
                                          info.filePath(),
                                          index));
        }
        return image;
    }

    default:
        break;
    }
    return {};
}

bool ElementsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_files.count())
        return false;
    if (role == ThumbnailRole) {
        const auto image = value.value<QImage>();
        if (image.isNull())
            m_overrideThumbnails.remove(index.row());
        else
            m_overrideThumbnails[index.row()] = image;
        emit dataChanged(index, index, {ThumbnailRole});
        return true;
    }
    return false;
}

Qt::ItemFlags ElementsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void ElementsModel::setDir(const QDir &dir)
{
    beginResetModel();
    m_dir = dir;
    m_overrideThumbnails.clear();
    m_files = dir.entryInfoList(kElementExtensions,
                                QDir::Files | QDir::Readable,
                                QDir::Name | QDir::IgnoreCase);
    endResetModel();
}

void ElementsModel::updateThumbnail(const QString &filePath,
                                    QImage &image,
                                    const QModelIndex &persistentIndex)
{
    const auto key = ElementsThumbnailTask::cacheKey(filePath);
    DB.putThumbnail(key, image);
    if (persistentIndex.isValid())
        emit dataChanged(persistentIndex, persistentIndex, {ThumbnailRole});
}
