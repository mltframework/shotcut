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

#ifndef HTMLGENERATORJOB_H
#define HTMLGENERATORJOB_H

#include "abstractjob.h"
#include "htmlgenerator.h"

#include <QDir>
#include <QTemporaryDir>

class HtmlGeneratorJob : public AbstractJob
{
    Q_OBJECT
public:
    HtmlGeneratorJob(const QString &name,
                     const QString &html,
                     const QString &outputPath,
                     int duration,
                     QThread::Priority priority = QThread::HighPriority);
    void start() override;

protected slots:
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus) override;
    void onReadyRead() override;

private slots:
    void onAnimationFramesReady();
    void onHtmlGeneratorProgress(float progress);
    void onOpenTriggered();

private:
    QString m_html;
    QString m_outputPath;
    int m_duration;
    HtmlGenerator *m_generator; // owned via parent QObject
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QString m_htmlFilePath;
    bool m_isGeneratingFrames;
    int m_previousPercent;
};

#endif // HTMLGENERATORJOB_H
