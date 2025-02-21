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

#include "subtitlesmodel.h"

#include "Logger.h"
#include "commands/subtitlecommands.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"

#include <QTimer>

#include <cmath>

static const quintptr NO_PARENT_ID = quintptr(-1);

enum Columns { COLUMN_TEXT = 0, COLUMN_START, COLUMN_END, COLUMN_DURATION, COLUMN_COUNT };

SubtitlesModel::SubtitlesModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_producer(nullptr)
    , m_commitTrack(-1)
{
    connect(&Settings, &ShotcutSettings::timeFormatChanged, this, [&]() {
        if (m_items.size() > 0) {
            beginResetModel();
            endResetModel();
        }
    });

    // The entire SRT document is written to an MLT property for storage.
    // So only write it after some delay to avoid frequent property updates.
    m_commitTimer = new QTimer(this);
    m_commitTimer->setSingleShot(true);
    m_commitTimer->setInterval(500);
    connect(m_commitTimer, &QTimer::timeout, this, [&]() {
        if (m_commitTrack == -1) {
            return;
        }
        commitToFeed(m_commitTrack);
        m_commitTrack = -1;
    });
}

SubtitlesModel::~SubtitlesModel() {}

void SubtitlesModel::load(Mlt::Producer *producer)
{
    beginResetModel();
    m_producer = producer;
    m_items.clear();
    m_tracks.clear();
    if (m_producer) {
        for (int i = 0; i < producer->filter_count(); i++) {
            QScopedPointer<Mlt::Filter> filter(producer->filter(i));
            if (!filter || !filter->is_valid()) {
                continue;
            }
            if (!::qstrcmp(filter->get("mlt_service"), "subtitle_feed")
                && filter->property_exists("text")) {
                SubtitleTrack track;
                track.name = QString::fromUtf8(filter->get("feed"));
                track.lang = QString::fromUtf8(filter->get("lang"));
                m_tracks.push_back(track);
                m_items.resize(m_tracks.size());
                Subtitles::SubtitleVector items = Subtitles::readFromSrtString(filter->get("text"));
                m_items[m_items.size() - 1] = QList(items.cbegin(), items.cend());
            }
        }
    }
    endResetModel();
    emit tracksChanged(m_tracks.size());
}

bool SubtitlesModel::isValid() const
{
    return m_producer && m_producer->is_valid();
}

int64_t SubtitlesModel::maxTime() const
{
    int64_t maxTime = 0;
    if (m_producer && m_producer->is_valid()) {
        maxTime = std::floor((double) m_producer->get_playtime() * 1000.0 / m_producer->get_fps());
    }
    return maxTime;
}

int SubtitlesModel::trackCount() const
{
    return m_tracks.size();
}

QModelIndex SubtitlesModel::trackModelIndex(int trackIndex) const
{
    return index(trackIndex, 0);
}

QList<SubtitlesModel::SubtitleTrack> SubtitlesModel::getTracks() const
{
    return m_tracks;
}

int SubtitlesModel::getTrackIndex(const QString &name)
{
    int index = -1;
    for (int i = 0; i < m_tracks.size(); i++) {
        if (m_tracks[i].name == name) {
            return i;
        }
    }
    return index;
}

SubtitlesModel::SubtitleTrack SubtitlesModel::getTrack(const QString &name)
{
    for (auto &track : m_tracks) {
        if (track.name == name) {
            return track;
        }
    }
    return SubtitlesModel::SubtitleTrack();
}

SubtitlesModel::SubtitleTrack SubtitlesModel::getTrack(int index)
{
    if (index < m_tracks.size() && index >= 0) {
        return m_tracks[index];
    }
    return SubtitlesModel::SubtitleTrack();
}

void SubtitlesModel::requestFeedCommit(int trackIndex)
{
    if (trackIndex >= m_items.size()) {
        LOG_ERROR() << "Invalid track Index" << trackIndex;
        return;
    }
    m_commitTimer->stop();
    if (m_commitTrack != -1 && m_commitTrack != trackIndex) {
        commitToFeed(m_commitTrack);
    }
    m_commitTrack = trackIndex;
    m_commitTimer->start();
}

void SubtitlesModel::commitToFeed(int trackIndex)
{
    if (trackIndex >= m_items.size()) {
        LOG_ERROR() << "Invalid track Index" << trackIndex;
        return;
    }
    int feedFilterIndex = 0;
    for (int i = 0; i < m_producer->filter_count(); i++) {
        QScopedPointer<Mlt::Filter> filter(m_producer->filter(i));
        if (!filter || !filter->is_valid()) {
            continue;
        }
        if (filter->get("mlt_service") == QStringLiteral("subtitle_feed")) {
            if (feedFilterIndex == trackIndex) {
                Subtitles::SubtitleVector items(m_items[trackIndex].constBegin(),
                                                m_items[trackIndex].constEnd());
                std::string text;
                Subtitles::writeToSrtString(text, items);
                filter->set("text", text.c_str());
                break;
            }
            feedFilterIndex++;
        }
    }
    emit modified();
}

void SubtitlesModel::addTrack(SubtitlesModel::SubtitleTrack &track)
{
    if (!m_producer) {
        LOG_DEBUG() << "No producer";
        return;
    }
    Subtitles::InsertTrackCommand *command = new Subtitles::InsertTrackCommand(*this,
                                                                               track,
                                                                               m_tracks.size());
    MAIN.undoStack()->push(command);
}

void SubtitlesModel::removeTrack(QString &name)
{
    if (!m_producer) {
        LOG_DEBUG() << "No producer";
        return;
    }
    for (int i = 0; i < m_tracks.size(); i++) {
        if (m_tracks[i].name == name) {
            Subtitles::RemoveTrackCommand *command = new Subtitles::RemoveTrackCommand(*this, i);
            MAIN.undoStack()->push(command);
            return;
        }
    }
    LOG_ERROR() << "Track not found:" << name;
}

void SubtitlesModel::editTrack(int trackIndex, SubtitlesModel::SubtitleTrack &track)
{
    if (!m_producer) {
        LOG_DEBUG() << "No producer";
        return;
    }
    Subtitles::EditTrackCommand *command = new Subtitles::EditTrackCommand(*this, track, trackIndex);
    MAIN.undoStack()->push(command);
}

int SubtitlesModel::itemCount(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        LOG_DEBUG() << "Invalid track index";
        return 0;
    }
    return m_items[trackIndex].size();
}

int64_t SubtitlesModel::endTime(int trackIndex) const
{
    int64_t endTime = 0;
    int count = m_items[trackIndex].size();
    if (count > 0) {
        endTime = m_items[trackIndex][count - 1].end;
    }
    return endTime;
}

QModelIndex SubtitlesModel::itemModelIndex(int trackIndex, int itemIndex) const
{
    return index(itemIndex, 0, index(trackIndex, 0));
}

int SubtitlesModel::itemIndexAtTime(int trackIndex, int64_t msTime) const
{
    int ret = -1;
    for (int i = 0; i < m_items[trackIndex].size(); i++) {
        if (m_items[trackIndex][i].start <= msTime && m_items[trackIndex][i].end >= msTime) {
            ret = i;
            break;
        } else if (m_items[trackIndex][i].start > msTime) {
            break;
        }
    }
    return ret;
}

int SubtitlesModel::itemIndexBeforeTime(int trackIndex, int64_t msTime) const
{
    int ret = -1;
    int itemCount = m_items[trackIndex].size();
    for (int i = 0; i < itemCount; i++) {
        if (m_items[trackIndex][i].start >= msTime) {
            ret = i - 1;
            break;
        }
    }
    if (ret == -1 && m_items[trackIndex].size() > 0
        && m_items[trackIndex][itemCount - 1].end < msTime) {
        ret = itemCount - 1;
    }
    return ret;
}

int SubtitlesModel::itemIndexAfterTime(int trackIndex, int64_t msTime) const
{
    int ret = -1;
    int itemCount = m_items[trackIndex].size();
    for (int i = 0; i < itemCount; i++) {
        if (m_items[trackIndex][i].start > msTime) {
            ret = i;
            break;
        }
    }
    return ret;
}

const Subtitles::SubtitleItem &SubtitlesModel::getItem(int trackIndex, int itemIndex) const
{
    return m_items[trackIndex][itemIndex];
}

void SubtitlesModel::importSubtitles(int trackIndex,
                                     int64_t msTime,
                                     QList<Subtitles::SubtitleItem> &items)
{
    if (!m_producer) {
        LOG_DEBUG() << "No producer";
        return;
    }
    Subtitles::OverwriteSubtitlesCommand *command
        = new Subtitles::OverwriteSubtitlesCommand(*this, trackIndex, items);
    command->setText(QObject::tr("Import %1 subtitle items").arg(items.size()));

    MAIN.undoStack()->push(command);
}

void SubtitlesModel::importSubtitlesToNewTrack(SubtitlesModel::SubtitleTrack &track,
                                               QList<Subtitles::SubtitleItem> &items)
{
    if (!m_producer) {
        LOG_DEBUG() << "No producer";
        return;
    }

    int trackIndex = m_tracks.size();

    MAIN.undoStack()->beginMacro(QObject::tr("Import %1 subtitle items").arg(items.size()));

    Subtitles::InsertTrackCommand *trackCommand = new Subtitles::InsertTrackCommand(*this,
                                                                                    track,
                                                                                    trackIndex);
    MAIN.undoStack()->push(trackCommand);

    Subtitles::OverwriteSubtitlesCommand *overwriteCommand
        = new Subtitles::OverwriteSubtitlesCommand(*this, trackIndex, items);
    MAIN.undoStack()->push(overwriteCommand);

    MAIN.undoStack()->endMacro();
}

void SubtitlesModel::exportSubtitles(const QString &filePath, int trackIndex) const
{
    if (!m_producer) {
        LOG_DEBUG() << "No producer";
        return;
    }
    Subtitles::SubtitleVector items(m_items[trackIndex].constBegin(),
                                    m_items[trackIndex].constEnd());
    Subtitles::writeToSrtFile(filePath.toUtf8().toStdString(), items);
}

void SubtitlesModel::overwriteItem(int trackIndex, const Subtitles::SubtitleItem &item)
{
    QList<Subtitles::SubtitleItem> items;
    items.append(item);
    Subtitles::OverwriteSubtitlesCommand *command
        = new Subtitles::OverwriteSubtitlesCommand(*this, trackIndex, items);
    MAIN.undoStack()->push(command);
}

void SubtitlesModel::appendItem(int trackIndex, const Subtitles::SubtitleItem &item)
{
    QList<Subtitles::SubtitleItem> items;
    items.append(item);
    Subtitles::OverwriteSubtitlesCommand *command
        = new Subtitles::OverwriteSubtitlesCommand(*this, trackIndex, items);
    command->setText(QObject::tr("Append subtitle"));
    MAIN.undoStack()->push(command);
}

void SubtitlesModel::removeItems(int trackIndex, int firstItemIndex, int lastItemIndex)
{
    int count = m_items[trackIndex].size();
    if (firstItemIndex < 0 || firstItemIndex >= count || lastItemIndex < 0
        || lastItemIndex >= count) {
        LOG_ERROR() << "Invalid index to remove" << firstItemIndex << lastItemIndex;
    }
    QList<Subtitles::SubtitleItem> items;
    for (int i = firstItemIndex; i <= lastItemIndex; i++) {
        items.append(m_items[trackIndex][i]);
    }
    Subtitles::RemoveSubtitlesCommand *command = new Subtitles::RemoveSubtitlesCommand(*this,
                                                                                       trackIndex,
                                                                                       items);
    MAIN.undoStack()->push(command);
}

void SubtitlesModel::setItemStart(int trackIndex, int itemIndex, int64_t msTime)
{
    if (trackIndex < 0 || trackIndex >= m_items.size()) {
        LOG_ERROR() << "Invalid track index" << trackIndex;
        return;
    }
    if (itemIndex < 0 || itemIndex >= m_items[trackIndex].size()) {
        LOG_ERROR() << "Invalid item index" << itemIndex;
        return;
    }
    if (msTime >= m_items[trackIndex][itemIndex].end) {
        LOG_ERROR() << "Start can not be greater than end" << msTime
                    << m_items[trackIndex][itemIndex].end;
        return;
    }
    if (itemIndex > 0 && msTime < m_items[trackIndex][itemIndex - 1].end) {
        LOG_ERROR() << "Start can not precede previous end" << msTime
                    << m_items[trackIndex][itemIndex - 1].end;
        return;
    }
    Subtitles::SetStartCommand *command
        = new Subtitles::SetStartCommand(*this, trackIndex, itemIndex, msTime);
    MAIN.undoStack()->push(command);
}

void SubtitlesModel::setItemEnd(int trackIndex, int itemIndex, int64_t msTime)
{
    if (trackIndex < 0 || trackIndex >= m_items.size()) {
        LOG_ERROR() << "Invalid track index" << trackIndex;
        return;
    }
    if (itemIndex < 0 || itemIndex >= m_items[trackIndex].size()) {
        LOG_ERROR() << "Invalid item index" << itemIndex;
        return;
    }
    if (msTime <= m_items[trackIndex][itemIndex].start) {
        LOG_ERROR() << "End can not be less than start" << msTime
                    << m_items[trackIndex][itemIndex].start;
        return;
    }
    if (itemIndex < (m_items[trackIndex].size() - 1)
        && msTime > m_items[trackIndex][itemIndex + 1].start) {
        LOG_ERROR() << "End can not be greater than next start" << msTime
                    << m_items[trackIndex][itemIndex + 1].start;
        return;
    }
    Subtitles::SetEndCommand *command
        = new Subtitles::SetEndCommand(*this, trackIndex, itemIndex, msTime);
    MAIN.undoStack()->push(command);
}

void SubtitlesModel::setText(int trackIndex, int itemIndex, const QString &text)
{
    Subtitles::SetTextCommand *command
        = new Subtitles::SetTextCommand(*this, trackIndex, itemIndex, text);
    MAIN.undoStack()->push(command);
}

void SubtitlesModel::moveItems(int trackIndex, int firstItemIndex, int lastItemIndex, int64_t msTime)
{
    int count = m_items[trackIndex].size();
    if (firstItemIndex < 0 || firstItemIndex >= count || lastItemIndex < 0
        || lastItemIndex >= count) {
        LOG_ERROR() << "Invalid index to move" << firstItemIndex << lastItemIndex;
    }
    QList<Subtitles::SubtitleItem> items;
    for (int i = firstItemIndex; i <= lastItemIndex; i++) {
        items.append(m_items[trackIndex][i]);
    }
    Subtitles::MoveSubtitlesCommand *command
        = new Subtitles::MoveSubtitlesCommand(*this, trackIndex, items, msTime);
    MAIN.undoStack()->push(command);
}

bool SubtitlesModel::validateMove(const QModelIndexList &items, int64_t msTime)
{
    if (items.size() <= 0) {
        return false;
    }
    if (msTime < 0) {
        return false;
    }
    int trackIndex = items[0].parent().row();
    // Check if there is a big enough gap at this location to move without conflict.
    int firstItemIndex = items[0].row();
    auto firstItem = m_items[trackIndex][firstItemIndex];
    int lastItemIndex = items[items.size() - 1].row();
    auto lastItem = m_items[trackIndex][lastItemIndex];
    int64_t duration = lastItem.end - firstItem.start;
    int64_t newEndTime = msTime + duration;
    int itemCount = m_items[trackIndex].size();
    int gapItemIndex = itemIndexAtTime(trackIndex, msTime);
    if (gapItemIndex == -1) {
        gapItemIndex = itemIndexAfterTime(trackIndex, msTime);
    }
    if (gapItemIndex >= 0) {
        while (gapItemIndex < itemCount) {
            if (gapItemIndex < firstItemIndex || gapItemIndex > lastItemIndex) {
                auto gapItem = m_items[trackIndex][gapItemIndex];
                if (gapItem.start >= newEndTime) {
                    break;
                } else if ((msTime >= gapItem.start && msTime < gapItem.end)
                           || (newEndTime > gapItem.start && newEndTime <= gapItem.end)
                           || (gapItem.start <= msTime && gapItem.end >= newEndTime)) {
                    return false;
                }
            }
            gapItemIndex++;
        }
    }
    return true;
}

void SubtitlesModel::doInsertTrack(const SubtitlesModel::SubtitleTrack &track, int trackIndex)
{
    if (trackIndex < 0 || trackIndex > m_tracks.size()) {
        LOG_ERROR() << "Invalid index" << trackIndex;
    }
    beginInsertRows(QModelIndex(), trackIndex, trackIndex);
    m_tracks.insert(trackIndex, track);
    m_items.insert(trackIndex, QList<Subtitles::SubtitleItem>());
    // Feed filters should be after all normalizers and before any user filters
    int filterIndex = m_producer->filter_count();
    for (int i = 0; i < m_producer->filter_count(); i++) {
        QScopedPointer<Mlt::Filter> filter(m_producer->filter(i));
        if (!filter || !filter->is_valid()) {
            continue;
        }
        if (!filter->get_int("_loader") && !filter->get_int(kShotcutHiddenProperty)) {
            filterIndex = i;
            break;
        }
    }
    Mlt::Filter newFilter(MLT.profile(), "subtitle_feed");
    newFilter.set("feed", track.name.toUtf8().constData());
    newFilter.set("lang", track.lang.toUtf8().constData());
    newFilter.set(kShotcutHiddenProperty, 1);
    m_producer->attach(newFilter);
    m_producer->move_filter(m_producer->filter_count() - 1, filterIndex);
    endInsertRows();
    emit tracksChanged(m_tracks.size());
    emit modified();
}

void SubtitlesModel::doRemoveTrack(int trackIndex)
{
    LOG_DEBUG() << trackIndex;
    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        LOG_ERROR() << "Invalid index" << trackIndex;
    }
    beginRemoveRows(QModelIndex(), trackIndex, trackIndex);
    m_tracks.remove(trackIndex);
    m_items.remove(trackIndex);
    int feedFilterIndex = 0;
    for (int i = 0; i < m_producer->filter_count(); i++) {
        QScopedPointer<Mlt::Filter> filter(m_producer->filter(i));
        if (!filter || !filter->is_valid()) {
            continue;
        }
        if (filter->get("mlt_service") == QStringLiteral("subtitle_feed")) {
            if (feedFilterIndex == trackIndex) {
                m_producer->detach(*filter);
                break;
            }
            feedFilterIndex++;
        }
    }
    endRemoveRows();
    emit tracksChanged(m_tracks.size());
    emit modified();
}

void SubtitlesModel::doEditTrack(const SubtitlesModel::SubtitleTrack &track, int trackIndex)
{
    if (trackIndex < 0 || trackIndex > m_tracks.size()) {
        LOG_ERROR() << "Invalid index" << trackIndex;
    }
    // Feed filters should be after all normalizers and before any user filters
    int filterIndex = -1;
    for (int i = 0; i < m_producer->filter_count(); i++) {
        QScopedPointer<Mlt::Filter> filter(m_producer->filter(i));
        if (filter && filter->is_valid()) {
            QString mlt_service = filter->get("mlt_service");
            if (mlt_service == QStringLiteral("subtitle_feed")) {
                filterIndex++;
                if (filterIndex == trackIndex) {
                    filter->set("feed", track.name.toUtf8().constData());
                    filter->set("lang", track.lang.toUtf8().constData());
                    break;
                }
            } else if (mlt_service == QStringLiteral("subtitle")) {
                // Modify subtitle burn-in filter if present
                if (filter->get("feed") == QString(m_tracks[trackIndex].name)) {
                    filter->set("feed", track.name.toUtf8().constData());
                }
            }
        }
    }

    if (filterIndex == -1) {
        LOG_ERROR() << "Subtitle filter not found" << trackIndex;
        return;
    }
    m_tracks[trackIndex] = track;
    emit dataChanged(index(trackIndex), index(trackIndex));
    emit tracksChanged(m_tracks.size());
    emit modified();
}

void SubtitlesModel::doRemoveSubtitleItems(int trackIndex,
                                           const QList<Subtitles::SubtitleItem> &subtitles)
{
    LOG_DEBUG() << trackIndex;
    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        LOG_ERROR() << "Invalid index" << trackIndex;
    }

    // Find the start and end indexes to remove
    int startIndex = -1;
    int endIndex = -1;
    for (int i = 0; i < m_items[trackIndex].size(); i++) {
        if (subtitles[0].start == m_items[trackIndex][i].start) {
            startIndex = i;
        }
        if (subtitles[subtitles.size() - 1].start == m_items[trackIndex][i].start) {
            endIndex = i;
        }
        if (startIndex > -1 && endIndex > -1) {
            break;
        }
    }
    if (startIndex == -1 || endIndex == -1) {
        LOG_ERROR() << "Failed to find items to remove" << startIndex << endIndex;
        return;
    }
    beginRemoveRows(index(trackIndex), startIndex, endIndex);
    m_items[trackIndex].remove(startIndex, endIndex - startIndex + 1);
    requestFeedCommit(trackIndex);
    endRemoveRows();
}

void SubtitlesModel::doInsertSubtitleItems(int trackIndex,
                                           const QList<Subtitles::SubtitleItem> &subtitles)
{
    LOG_DEBUG() << trackIndex;
    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        LOG_ERROR() << "Invalid index" << trackIndex;
        return;
    }
    int oldSize = m_items[trackIndex].size();
    // Find the insert index
    int insertIndex = oldSize;
    for (int i = 0; i < oldSize; i++) {
        if (m_items[trackIndex][i].start >= subtitles[0].start) {
            insertIndex = i;
            break;
        }
    }
    QModelIndex parent = index(trackIndex);
    beginInsertRows(parent, insertIndex, insertIndex + subtitles.size() - 1);
    // Resize the list to fit the new items
    m_items[trackIndex].resize(m_items[trackIndex].size() + subtitles.size());
    // Move existing items to make room for the new items
    if (insertIndex < oldSize) {
        for (int i = oldSize - 1; i >= insertIndex; i--) {
            m_items[trackIndex].move(i, i + subtitles.size());
        }
    }
    // Put in the new items
    for (int i = 0; i < subtitles.size(); i++) {
        m_items[trackIndex][insertIndex + i] = subtitles[i];
    }
    requestFeedCommit(trackIndex);
    endInsertRows();
}

void SubtitlesModel::doSetText(int trackIndex, int itemIndex, const QString &text)
{
    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        LOG_ERROR() << "Invalid track" << trackIndex;
        return;
    }
    if (itemIndex >= 0 && itemIndex < m_items[trackIndex].size()) {
        m_items[trackIndex][itemIndex].text = text.toStdString();
        requestFeedCommit(trackIndex);
        QModelIndex parent = index(trackIndex);
        emit dataChanged(index(itemIndex, COLUMN_TEXT, parent),
                         index(itemIndex, COLUMN_TEXT, parent));
    } else {
        LOG_ERROR() << "Invalid index" << itemIndex;
    }
}

void SubtitlesModel::doSetTime(int trackIndex, int itemIndex, int64_t startTime, int64_t endTime)
{
    if (trackIndex < 0 || trackIndex >= m_tracks.size()) {
        LOG_ERROR() << "Invalid track" << trackIndex;
        return;
    }
    if (itemIndex >= 0 && itemIndex < m_items[trackIndex].size()) {
        m_items[trackIndex][itemIndex].start = startTime;
        m_items[trackIndex][itemIndex].end = endTime;
        requestFeedCommit(trackIndex);
        QModelIndex parent = index(trackIndex);
        emit dataChanged(index(itemIndex, COLUMN_START, parent),
                         index(itemIndex, COLUMN_DURATION, parent));
    } else {
        LOG_ERROR() << "Invalid index" << itemIndex;
    }
}

int SubtitlesModel::rowCount(const QModelIndex &parent) const
{
    int count = 0;
    if (!parent.isValid()) {
        count = m_tracks.size();
    } else if (parent.internalId() != NO_PARENT_ID) {
        count = 0;
    } else if (parent.row() >= 0 && parent.row() < m_items.size()) {
        count = m_items[parent.row()].size();
    }
    return count;
}

int SubtitlesModel::columnCount(const QModelIndex &parent) const
{
    return COLUMN_COUNT;
}

QVariant SubtitlesModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    switch (role) {
    case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::DecorationRole:
    case Qt::FontRole:
    case Qt::TextAlignmentRole:
    case Qt::CheckStateRole:
    case Qt::SizeHintRole:
    case Qt::BackgroundRole:
    case Qt::ForegroundRole:
        return result;
    }

    if (!m_producer) {
        LOG_DEBUG() << "No Producer: " << index.row() << index.column() << role;
        return result;
    }

    if (!index.parent().isValid()) {
        // Subtitle track
        if (index.row() >= 0 && index.row() < m_tracks.size()) {
            result = m_tracks[index.row()].name;
        } else {
            LOG_ERROR() << "Invalid root index: " << index.row() << index.column() << role;
        }
        return result;
    }

    // Subtitle item
    if (!index.isValid() || index.column() < 0 || index.column() >= COLUMN_COUNT || index.row() < 0
        || index.row() >= itemCount(index.parent().row())) {
        LOG_ERROR() << "Invalid Index: " << index.row() << index.column() << role;
        return result;
    }

    Subtitles::SubtitleItem item = m_items[index.parent().row()][index.row()];

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case COLUMN_TEXT:
            result = QString::fromStdString(item.text).replace('\n', ' ');
            break;
        case COLUMN_START: {
            mlt_position frames = item.start * MLT.profile().fps() / 1000;
            result = QString(m_producer->frames_to_time(frames, Settings.timeFormat()));
            break;
        }
        case COLUMN_END: {
            mlt_position frames = item.end * MLT.profile().fps() / 1000;
            result = QString(m_producer->frames_to_time(frames, Settings.timeFormat()));
            break;
        }
        case COLUMN_DURATION: {
            mlt_position frames = (item.end - item.start) * MLT.profile().fps() / 1000;
            result = QString(m_producer->frames_to_time(frames, Settings.timeFormat()));
            break;
        }
        default:
            LOG_ERROR() << "Invalid Column" << index.column() << role;
            break;
        }
        break;
    case TextRole:
        result = QString::fromStdString(item.text);
        break;
    case StartRole:
        result = (qlonglong) item.start;
        break;
    case EndRole:
        result = (qlonglong) item.end;
        break;
    case DurationRole:
        result = (qlonglong) (item.end - item.start);
        break;
    case SimpleText:
        result = QString::fromStdString(item.text).replace('\n', ' ');
        break;
    case StartFrameRole:
        result = (int) std::round(item.start * MLT.profile().fps() / 1000);
        break;
    case EndFrameRole:
        result = (int) std::round(item.end * MLT.profile().fps() / 1000);
        break;
    case SiblingCountRole:
        result = m_items[index.parent().row()].size();
        break;
    default:
        LOG_ERROR() << "Invalid Role" << index.row() << index.column() << roleNames()[role] << role;
        break;
    }
    return result;
}

QVariant SubtitlesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
        case COLUMN_TEXT:
            return tr("Text");
        case COLUMN_START:
            return tr("Start");
        case COLUMN_END:
            return tr("End");
        case COLUMN_DURATION:
            return tr("Duration");
        default:
            break;
        }
    }
    return QVariant();
}

QModelIndex SubtitlesModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex result;
    if (parent.isValid()) {
        // subtitle item
        result = createIndex(row, column, parent.row());
    } else if (row >= 0 && row < m_tracks.size() && column == 0) {
        // subtitle track
        result = createIndex(row, column, NO_PARENT_ID);
    }
    return result;
}

QModelIndex SubtitlesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.internalId() == NO_PARENT_ID)
        return QModelIndex();
    else
        return createIndex(index.internalId(), 0, NO_PARENT_ID);
}

QHash<int, QByteArray> SubtitlesModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[TextRole] = "text";
    roles[StartRole] = "start";
    roles[EndRole] = "end";
    roles[DurationRole] = "duration";
    roles[SimpleText] = "simpleText";
    roles[StartFrameRole] = "startFrame";
    roles[EndFrameRole] = "endFrame";
    roles[SiblingCountRole] = "siblingCount";
    return roles;
}
