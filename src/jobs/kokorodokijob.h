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
#ifndef KOKORODOKIJOB_H
#define KOKORODOKIJOB_H

#include "abstractjob.h"

class KokorodokiJob : public AbstractJob
{
    Q_OBJECT
public:
    KokorodokiJob(const QString &inputFile,
                  const QString &outputFile,
                  const QString &language,
                  const QString &voice,
                  double speed,
                  QThread::Priority priority = Settings.jobPriority());
    ~KokorodokiJob() override;
    // Performs Docker availability/image checks and optionally pulls the image.
    // Calls 'schedule' once the image is ready (or immediately if already current).
    // 'parent' is used for dialogs.
    static void prepareAndRun(QWidget *parent, std::function<void()> schedule);

public slots:
    void start() override;

protected slots:
    void onReadyRead() override;

private:
    void onOpenTriggered();
    QString m_inputFile;
    QString m_outputFile;
    QString m_language;
    QString m_voice;
    double m_speed;
    QString m_lastTimecode;
    QString m_lastLogLine;
};
#endif // KOKORODOKIJOB_H
