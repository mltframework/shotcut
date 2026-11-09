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

#include "api/apiplayer.h"

#include "api/apiutil.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "player.h"
#include "videowidget.h"

namespace Shotcut {
namespace Api {

ApiPlayer::ApiPlayer(QObject *parent)
    : ApiObject(parent)
{
    // Connected after Player's own connection, so Player::position() is already updated.
    if (auto videoWidget = qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget()))
        connect(videoWidget, &Mlt::VideoWidget::frameDisplayed, this, &ApiPlayer::onFrameDisplayed);

    auto player = MAIN.player();
    connect(player, &Player::seeked, this, &ApiPlayer::onFrameDisplayed);
    connect(player, &Player::played, this, &ApiPlayer::changed);
    connect(player, &Player::paused, this, &ApiPlayer::changed);
    connect(player, &Player::stopped, this, &ApiPlayer::changed);
    connect(player, &Player::inChanged, this, &ApiPlayer::changed);
    connect(player, &Player::outChanged, this, &ApiPlayer::changed);
    connect(player, &Player::tabIndexChanged, this, &ApiPlayer::changed);
    connect(&MAIN, &MainWindow::producerOpened, this, &ApiPlayer::changed);
}

int ApiPlayer::position() const
{
    return MAIN.player()->position();
}

int ApiPlayer::duration() const
{
    return MLT.producer() && MLT.producer()->is_valid() ? MLT.producer()->get_playtime() : 0;
}

int ApiPlayer::inPoint() const
{
    return MLT.producer() && MLT.producer()->is_valid() ? MLT.producer()->get_in() : 0;
}

int ApiPlayer::outPoint() const
{
    return MLT.producer() && MLT.producer()->is_valid() ? MLT.producer()->get_out() : 0;
}

bool ApiPlayer::isPlaying() const
{
    return MLT.producer() && !MLT.isPaused();
}

QString ApiPlayer::source() const
{
    if (MLT.isMultitrack())
        return QStringLiteral("timeline");
    if (MLT.isPlaylist())
        return QStringLiteral("playlist");
    if (MLT.isClip())
        return QStringLiteral("clip");
    return QStringLiteral("none");
}

QString ApiPlayer::tab() const
{
    return MAIN.player()->tabIndex() == Player::ProjectTabIndex ? QStringLiteral("project")
                                                                : QStringLiteral("source");
}

QVariantMap ApiPlayer::producer() const
{
    QVariantMap result = producerSnapshot(MLT.producer());
    if (!result.isEmpty())
        result["source"] = source();
    return result;
}

QVariant ApiPlayer::seek(int position)
{
    if (!MLT.producer() || !MLT.producer()->is_valid())
        return fail(ErrorCode::Unavailable, tr("The player is not showing anything."));
    if (!MLT.isSeekable())
        return fail(ErrorCode::Unavailable, tr("What the player is showing cannot seek."));
    if (position < 0)
        return fail(ErrorCode::InvalidArgument, tr("Position cannot be negative."));
    const int last = MLT.isMultitrack() ? duration() : qMax(0, duration() - 1);
    position = qMin(position, last);
    MAIN.player()->seek(position);
    return position;
}

QVariant ApiPlayer::play(const QVariantMap &options)
{
    if (!checkOptions("play", options))
        return QVariant();
    if (!MLT.producer() || !MLT.producer()->is_valid())
        return fail(ErrorCode::Unavailable, tr("The player is not showing anything."));
    const double speed = options.value("speed", 1.0).toDouble();
    if (speed == 0.0 || qAbs(speed) > 32.0)
        return fail(ErrorCode::InvalidArgument, tr("speed must be nonzero and between -32 and 32."));
    MAIN.player()->play(speed);
    return QVariant();
}

QVariant ApiPlayer::pause()
{
    if (!MLT.producer() || !MLT.producer()->is_valid())
        return fail(ErrorCode::Unavailable, tr("The player is not showing anything."));
    MAIN.player()->pause();
    return position();
}

void ApiPlayer::onFrameDisplayed()
{
    const int current = position();
    if (current != m_lastPosition) {
        m_lastPosition = current;
        emit positionChanged();
    }
}

} // namespace Api
} // namespace Shotcut
