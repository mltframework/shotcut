/*
 * Copyright (c) 2025 Meltytech, LLC
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

#include "keyframestask.h"

#include "Logger.h"
#include "database.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"
#include "util.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QMutex>
#include <QProcess>
#include <QMap>
#include <QRgb>
#include <QThreadPool>

// Pixel value used to pad unused columns in the DB cache image.
static constexpr QRgb kPaddingSentinel = 0xFFFFFFFFu;

static QList<KeyframesTask *> tasksList;
static QMutex tasksListMutex;

static void deleteKeyframesMap(void *data)
{
    delete static_cast<QMap<int, QVector<int>> *>(data);
}

static bool isVideoProducer(Mlt::Producer *p)
{
    // Exclude audio-only producers
    if (p->get("video_index") && p->get_int("video_index") < 0)
        return false;
    return true;
}

KeyframesTask::KeyframesTask(Mlt::Producer &producer)
    : QRunnable()
    , m_isCanceled(false)
    , m_isForce(false)
{
    m_producers << new Mlt::Producer(producer);
}

KeyframesTask::~KeyframesTask()
{
    qDeleteAll(m_producers);
}

void KeyframesTask::start(Mlt::Producer &producer, bool force)
{
    if (!producer.is_valid())
        return;

    QString serviceName = QString::fromLatin1(producer.get("mlt_service"));
    // Accept avformat*, avformat-novalidate, and chain (chain wraps avformat for Source tab)
    bool isAvformat = serviceName.startsWith("avformat");
    bool isChain = (serviceName == "chain");
    if (!isAvformat && !isChain)
        return;

    // Skip audio-only
    if (!isVideoProducer(&producer))
        return;

    // Require a resource path for ffprobe
    const char *resource = producer.get("resource");
    if (!resource || !resource[0])
        return;

    // Skip if already have valid keyframes and not forcing
    if (!force && producer.get_data(kKeyframesProperty))
        return;

    KeyframesTask *task = new KeyframesTask(producer);
    tasksListMutex.lock();
    foreach (KeyframesTask *t, tasksList) {
        if (*t == *task) {
            // Merge: add this producer to the existing task's notify list
            delete task;
            task = nullptr;
            t->m_producers << new Mlt::Producer(producer);
            break;
        }
    }
    if (task) {
        task->m_isForce = force;
        tasksList << task;
        QThreadPool::globalInstance()->start(task);
    }
    tasksListMutex.unlock();
}

void KeyframesTask::closeAll()
{
    tasksListMutex.lock();
    while (!tasksList.isEmpty()) {
        KeyframesTask *task = tasksList.first();
        task->m_isCanceled = true;
        tasksList.removeFirst();
    }
    tasksListMutex.unlock();
}

bool KeyframesTask::operator==(const KeyframesTask &b) const
{
    if (!m_producers.isEmpty() && !b.m_producers.isEmpty()) {
        const char *aRes = m_producers.first()->get("resource");
        const char *bRes = b.m_producers.first()->get("resource");
        return aRes && bRes && !qstrcmp(aRes, bRes);
    }
    return false;
}

void KeyframesTask::setProperty(const QMap<int, QVector<int>> &keyframes)
{
    foreach (Mlt::Producer *p, m_producers) {
        auto *copy = new QMap<int, QVector<int>>(keyframes);
        p->lock();
        p->set(kKeyframesProperty, copy, 0, deleteKeyframesMap);
        p->unlock();
    }
}

QString KeyframesTask::cacheKey() const
{
    Mlt::Producer *producer = m_producers.first();
    QString key;
    if (producer->get(kShotcutHashProperty)) {
        key = QStringLiteral("%1 keyframes").arg(producer->get(kShotcutHashProperty));
    } else {
        key = QStringLiteral("%1 keyframes").arg(producer->get("resource"));
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(key.toUtf8());
        key = hash.result().toHex();
    }
    return key;
}

void KeyframesTask::run()
{
    // Remove from the global task list unconditionally so that no exit path
    // (early return, cancellation, or normal completion) can leave a dangling
    // pointer that the next start() call would dereference.
    tasksListMutex.lock();
    tasksList.removeOne(this);
    tasksListMutex.unlock();

    if (m_isCanceled || m_producers.isEmpty())
        return;

    Mlt::Producer *producer = m_producers.first();
    QString resource = QString::fromUtf8(producer->get("resource"));
    if (resource.isEmpty())
        return;

    const QString key = cacheKey();
    QImage cachedImage = DB.getThumbnail(key);

    if (!cachedImage.isNull() && !m_isForce) {
        // Restore from DB cache.
        // Image format: col 0 = stream index (as QRgb uint), cols 1..W-1 = keyframe frame numbers.
        // Unused columns are filled with kPaddingSentinel. kIntraOnlySentinel in a slot = intra-only.
        QMap<int, QVector<int>> kfMap;
        for (int y = 0; y < cachedImage.height(); ++y) {
            int streamIdx = static_cast<int>(static_cast<QRgb>(cachedImage.pixel(0, y)));
            QVector<int> kf;
            for (int x = 1; x < cachedImage.width(); ++x) {
                QRgb val = static_cast<QRgb>(cachedImage.pixel(x, y));
                if (val == kPaddingSentinel)
                    break;
                kf.append(static_cast<int>(val));
            }
            if (!kf.isEmpty())
                kfMap[streamIdx] = kf;
        }
        if (!kfMap.isEmpty()) {
            setProperty(kfMap);
            int total = 0;
            for (const auto &kf : kfMap)
                total += kf.size();
            LOG_DEBUG() << "KeyframesTask: loaded" << total << "keyframes from cache for"
                        << resource;
        }
        return;
    }

    // Locate ffprobe alongside the application
    QString ffprobePath = QFileInfo(qApp->applicationDirPath(), "ffprobe").absoluteFilePath();
    QStringList args;
    args << "-v"
         << "quiet"
         << "-select_streams"
         << "v"
         << "-show_packets"
         << "-show_entries"
         << "packet=stream_index,pts_time,flags"
         << "-of"
         << "csv" << resource;

    QProcess proc;
    proc.start(ffprobePath, args, QIODevice::ReadOnly);
    if (!proc.waitForStarted(5000)) {
        LOG_WARNING() << "KeyframesTask: ffprobe failed to start for" << resource;
        return;
    }

    double fps = 0.0;
    if (producer->get_double("meta.media.frame_rate_den") != 0.0) {
        fps = producer->get_double("meta.media.frame_rate_num")
              / producer->get_double("meta.media.frame_rate_den");
    }
    if (fps <= 0.0)
        fps = MLT.profile().fps();

    // Per-stream state: keyed by video stream index from ffprobe output.
    struct StreamState
    {
        QVector<int> keyframes;
        double firstPtsTime{-1.0};
        int packetCount{0};
        bool mightBeIntraOnly{true};
    };
    QMap<int, StreamState> streams;
    const int INTRA_ONLY_CHECK = 60;

    QElapsedTimer updateTimer;
    updateTimer.start();

    while (!m_isCanceled) {
        if (!proc.waitForReadyRead(1000)) {
            if (proc.state() == QProcess::NotRunning)
                break;
            continue;
        }
        while (proc.canReadLine()) {
            if (m_isCanceled) {
                proc.kill();
                proc.waitForFinished(1000);
                return;
            }
            const QByteArray line = proc.readLine().trimmed();
            // Expected CSV line: "packet,<stream_index>,<pts_time>,<flags>"
            const QList<QByteArray> parts = line.split(',');
            if (parts.size() < 4 || parts[0] != "packet")
                continue;

            bool okIdx = false, okPts = false;
            const int streamIdx = parts[1].toInt(&okIdx);
            const double ptsTime = parts[2].toDouble(&okPts);
            if (!okIdx || !okPts)
                continue;

            StreamState &s = streams[streamIdx];
            if (s.firstPtsTime < 0.0)
                s.firstPtsTime = ptsTime;

            const bool isKeyframe = parts[3].contains('K');
            ++s.packetCount;

            if (s.mightBeIntraOnly && !isKeyframe)
                s.mightBeIntraOnly = false;

            if (s.mightBeIntraOnly && s.packetCount >= INTRA_ONLY_CHECK) {
                // Stream confirmed intra-only; tag it and stop collecting keyframes for it.
                s.keyframes = {kIntraOnlySentinel};
            } else if (isKeyframe && (s.keyframes.isEmpty() || s.keyframes.last() != kIntraOnlySentinel)) {
                const int frameNum = qRound((ptsTime - s.firstPtsTime) * fps);
                s.keyframes.append(frameNum);
            }

            // Early exit when every observed video stream is confirmed intra-only.
            bool allIntraOnly = !streams.isEmpty();
            for (const auto &st : std::as_const(streams)) {
                if (!st.mightBeIntraOnly || st.packetCount < INTRA_ONLY_CHECK) {
                    allIntraOnly = false;
                    break;
                }
            }
            if (allIntraOnly) {
                proc.kill();
                proc.waitForFinished(1000);
                QMap<int, QVector<int>> kfMap;
                for (auto it = streams.cbegin(); it != streams.cend(); ++it)
                    kfMap[it.key()] = {kIntraOnlySentinel};
                setProperty(kfMap);
                const QList<int> intraKeys = kfMap.keys();
                QImage img(2, intraKeys.size(), QImage::Format_ARGB32);
                img.fill(kPaddingSentinel);
                for (int y = 0; y < intraKeys.size(); ++y) {
                    img.setPixel(0, y, static_cast<QRgb>(intraKeys[y]));
                    img.setPixel(1, y, static_cast<QRgb>(static_cast<unsigned>(kIntraOnlySentinel)));
                }
                DB.putThumbnail(key, img);
                LOG_DEBUG() << "KeyframesTask: intra-only detected for" << resource;
                return;
            }
        }

        // Periodic intermediate result every 3 seconds
        if (updateTimer.elapsed() >= 3000) {
            QMap<int, QVector<int>> partial;
            for (auto it = streams.cbegin(); it != streams.cend(); ++it) {
                if (!it->keyframes.isEmpty())
                    partial[it.key()] = it->keyframes;
            }
            if (!partial.isEmpty()) {
                setProperty(partial);
                updateTimer.restart();
            }
        }
    }

    proc.waitForFinished(2000);
    if (proc.exitStatus() != QProcess::NormalExit)
        proc.kill();

    // Mark any stream that finished with all-keyframe packets but fewer than
    // INTRA_ONLY_CHECK (e.g. very short clip) as intra-only.
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        if (it->mightBeIntraOnly && it->keyframes.isEmpty())
            it->keyframes = {kIntraOnlySentinel};
    }

    QMap<int, QVector<int>> kfMap;
    for (auto it = streams.cbegin(); it != streams.cend(); ++it) {
        if (!it->keyframes.isEmpty())
            kfMap[it.key()] = it->keyframes;
    }

    if (m_isCanceled || kfMap.isEmpty())
        return;

    // Persist to DB.
    // Image layout: col 0 = stream index (as QRgb uint), cols 1..W-1 = keyframe frame numbers.
    // Unused columns are filled with kPaddingSentinel.
    int maxKF = 0;
    for (const auto &kf : kfMap)
        maxKF = qMax(maxKF, kf.size());
    const QList<int> sortedKeys = kfMap.keys();
    QImage img(1 + maxKF, sortedKeys.size(), QImage::Format_ARGB32);
    img.fill(kPaddingSentinel);
    for (int y = 0; y < sortedKeys.size(); ++y) {
        const int si = sortedKeys[y];
        img.setPixel(0, y, static_cast<QRgb>(si));
        const QVector<int> &kf = kfMap[si];
        for (int x = 0; x < kf.size(); ++x)
            img.setPixel(x + 1, y, static_cast<QRgb>(kf[x]));
    }
    DB.putThumbnail(key, img);

    setProperty(kfMap);
    int total = 0;
    for (const auto &kf : kfMap)
        total += kf.size();
    LOG_DEBUG() << "KeyframesTask: stored" << total << "keyframes for" << resource
                << "across" << kfMap.size() << "video stream(s)";
}
