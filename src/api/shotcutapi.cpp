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

#include "api/shotcutapi.h"

#include "api/transaction.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "settings.h"

#include <Mlt.h>
#include <QCoreApplication>
#include <QTimer>
#include <QUndoStack>

namespace Shotcut {
namespace Api {

namespace {

void setTimeProfile(Mlt::Properties &properties)
{
    properties.set("_profile", MLT.profile().get_profile(), 0);
}

} // namespace

ShotcutApi &ShotcutApi::singleton()
{
    // Created lazily, after MainWindow and its docks exist.
    static ShotcutApi *instance = new ShotcutApi(&MAIN);
    return *instance;
}

ShotcutApi::ShotcutApi(QObject *parent)
    : ApiObject(parent)
    , m_project(new ApiProject(this))
    , m_player(new ApiPlayer(this))
    , m_timeline(new ApiTimeline(this))
    , m_playlist(new ApiPlaylist(this))
    , m_markers(new ApiMarkers(this))
    , m_filters(new ApiFilters(this))
    , m_undo(new ApiUndo(this))
    , m_jobs(new ApiJobs(this))
{
    connect(MAIN.undoStack(), &QUndoStack::indexChanged, this, &ShotcutApi::bumpRevision);
    connect(&MAIN, &MainWindow::producerOpened, this, &ShotcutApi::bumpRevision);
    connect(&MAIN, &MainWindow::profileChanged, this, &ShotcutApi::bumpRevision);
    connect(m_timeline, &ApiTimeline::changed, this, &ShotcutApi::bumpRevision);
    connect(m_playlist, &ApiPlaylist::changed, this, &ShotcutApi::bumpRevision);
    connect(m_markers, &ApiMarkers::changed, this, &ShotcutApi::bumpRevision);
    connect(m_filters, &ApiFilters::changed, this, &ShotcutApi::bumpRevision);
    // Changes inside a transaction count only if it commits; a rollback leaves the project as it was.
    connect(&Transaction::singleton(),
            &Transaction::closed,
            this,
            [this](const QString &, Transaction::Result result) {
                if (result == Transaction::Result::Committed)
                    bumpRevision();
            });
}

QString ShotcutApi::version() const
{
    return QCoreApplication::applicationVersion();
}

QString ShotcutApi::apiVersion() const
{
    return QStringLiteral("0.1");
}

QString ShotcutApi::timecode(int frames, const QString &format) const
{
    mlt_time_format timeFormat = Settings.timeFormat();
    if (format == QLatin1String("smpte"))
        timeFormat = mlt_time_smpte_df;
    else if (format == QLatin1String("smpte_ndf"))
        timeFormat = mlt_time_smpte_ndf;
    else if (format == QLatin1String("clock"))
        timeFormat = mlt_time_clock;
    else if (format == QLatin1String("frames"))
        timeFormat = mlt_time_frames;
    else if (!format.isEmpty())
        return fail(ErrorCode::InvalidArgument, tr("Unknown time format: %1").arg(format)).toString();
    Mlt::Properties properties;
    setTimeProfile(properties);
    return QString::fromLatin1(properties.frames_to_time(frames, timeFormat));
}

int ShotcutApi::frames(const QString &timecode) const
{
    Mlt::Properties properties;
    setTimeProfile(properties);
    const int result = properties.time_to_frames(timecode.toLatin1().constData());
    if (result < 0)
        fail(ErrorCode::InvalidArgument, tr("Invalid time: %1").arg(timecode));
    return result;
}

void ShotcutApi::bumpRevision()
{
    if (Transaction::singleton().isOpen())
        return;
    // Several signals fire per edit; count and notify once per event loop pass.
    if (!m_revisionNotifyPending) {
        ++m_revision;
        m_revisionNotifyPending = true;
        QTimer::singleShot(0, this, [this]() {
            m_revisionNotifyPending = false;
            emit revisionChanged();
        });
    }
}

} // namespace Api
} // namespace Shotcut
