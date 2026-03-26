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

#include "linuxtools.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariantMap>

static const QLatin1String kDesktopFile("application://org.shotcut.Shotcut.desktop");
static const QLatin1String kDbusPath("/com/canonical/unity/launcherentry/1");
static const QLatin1String kDbusInterface("com.canonical.Unity.LauncherEntry");

static void sendLauncherUpdate(const QVariantMap &props)
{
    QDBusMessage msg = QDBusMessage::createSignal(kDbusPath, kDbusInterface, QStringLiteral("Update"));
    msg << kDesktopFile << props;
    QDBusConnection::sessionBus().send(msg);
}

void linuxSetLauncherProgress(int percent)
{
    QVariantMap props;
    props[QStringLiteral("progress")] = percent / 100.0;
    props[QStringLiteral("progress-visible")] = true;
    props[QStringLiteral("urgent")] = false;
    sendLauncherUpdate(props);
}

void linuxPauseLauncherProgress(int percent)
{
    QVariantMap props;
    props[QStringLiteral("progress")] = percent / 100.0;
    props[QStringLiteral("progress-visible")] = true;
    props[QStringLiteral("urgent")] = false;
    sendLauncherUpdate(props);
}

void linuxResetLauncherProgress()
{
    QVariantMap props;
    props[QStringLiteral("progress-visible")] = false;
    props[QStringLiteral("urgent")] = false;
    sendLauncherUpdate(props);
}

void linuxFinishLauncherProgress(bool isSuccess, bool stopped)
{
    QVariantMap props;
    props[QStringLiteral("progress-visible")] = false;
    props[QStringLiteral("urgent")] = !isSuccess && !stopped;
    sendLauncherUpdate(props);
}
