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

#include "dockerpulljob.h"

#include "Logger.h"
#include "settings.h"

#include <QRegularExpression>

DockerPullJob::DockerPullJob(const QString &imageRef, QThread::Priority priority)
    : AbstractJob(QObject::tr("docker pull %1").arg(imageRef), priority)
    , m_imageRef(imageRef)
{
    setTarget(imageRef);
}

DockerPullJob::~DockerPullJob()
{
    LOG_DEBUG() << "DockerPullJob destroyed";
}

void DockerPullJob::start()
{
    QString docker = Settings.dockerPath();
    QStringList args{QStringLiteral("pull"), m_imageRef};
    setProcessChannelMode(QProcess::MergedChannels);
    LOG_DEBUG() << docker + " " + args.join(' ');
    AbstractJob::start(docker, args);
    emit progressUpdated(m_item, 0);
}

void DockerPullJob::onReadyRead()
{
    QString msg;
    do {
        msg = readLine();
        if (!msg.isEmpty()) {
            appendToLog(msg);
            // Typical docker pull progress lines contain something like:
            // Digest: sha256:...
            // Status: Downloaded newer image for repository:tag
            // or layer progress lines: <id>: Pull complete
            // There's no simple percentage, so we approximate by counting 'Pull complete'.
            static int totalComplete = 0;
            if (msg.contains("Pull complete")) {
                ++totalComplete;
                int percent = qMin(99, totalComplete * 5); // heuristic
                if (percent != m_previousPercent) {
                    emit progressUpdated(m_item, percent);
                    m_previousPercent = percent;
                }
            } else if (msg.startsWith("Status: ")) {
                emit progressUpdated(m_item, 100);
            }
        }
    } while (!msg.isEmpty());
}
