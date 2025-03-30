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

#ifndef FIND_ANALYSIS_FILTER_PARSER_H
#define FIND_ANALYSIS_FILTER_PARSER_H

#include "qmltypes/qmlapplication.h"

#include <Mlt.h>
#include <QFile>
#include <QList>
#include <QString>

class FindAnalysisFilterParser : public Mlt::Parser
{
private:
    QList<Mlt::Filter> m_filters;
    bool m_skipAnalyzed;

public:
    FindAnalysisFilterParser()
        : Mlt::Parser()
        , m_skipAnalyzed(true)
    {}

    QList<Mlt::Filter> &filters() { return m_filters; }

    void skipAnalyzed(bool skip) { m_skipAnalyzed = skip; }

    int on_start_filter(Mlt::Filter *filter)
    {
        QString serviceName = filter->get("mlt_service");
        if (serviceName == "loudness" || serviceName == "vidstab") {
            // If the results property does not exist, empty, or file does not exist.
            QString results = filter->get("results");
            if (results.isEmpty() || !m_skipAnalyzed) {
                if (serviceName == "vidstab") {
                    // vidstab requires a filename, which is only available when using a project folder.
                    QString filename = filter->get("filename");
                    if (filename.isEmpty() || filename.endsWith("vidstab.trf")) {
                        filename = QmlApplication::getNextProjectFile("stab");
                    }
                    if (!filename.isEmpty()) {
                        filter->set("filename", filename.toUtf8().constData());
                        m_filters << Mlt::Filter(*filter);

                        // Touch file to prevent overwriting the same file
                        QFile file(filename);
                        file.open(QIODevice::WriteOnly);
                        file.resize(0);
                        file.close();
                    }
                } else {
                    m_filters << Mlt::Filter(*filter);
                }
            }
        }
        return 0;
    }
    int on_start_producer(Mlt::Producer *) { return 0; }
    int on_end_producer(Mlt::Producer *) { return 0; }
    int on_start_playlist(Mlt::Playlist *) { return 0; }
    int on_end_playlist(Mlt::Playlist *) { return 0; }
    int on_start_tractor(Mlt::Tractor *) { return 0; }
    int on_end_tractor(Mlt::Tractor *) { return 0; }
    int on_start_multitrack(Mlt::Multitrack *) { return 0; }
    int on_end_multitrack(Mlt::Multitrack *) { return 0; }
    int on_start_track() { return 0; }
    int on_end_track() { return 0; }
    int on_end_filter(Mlt::Filter *) { return 0; }
    int on_start_transition(Mlt::Transition *) { return 0; }
    int on_end_transition(Mlt::Transition *) { return 0; }
    int on_start_chain(Mlt::Chain *) { return 0; }
    int on_end_chain(Mlt::Chain *) { return 0; }
    int on_start_link(Mlt::Link *) { return 0; }
    int on_end_link(Mlt::Link *) { return 0; }
};

#endif // FIND_ANALYSIS_FILTER_PARSER_H
