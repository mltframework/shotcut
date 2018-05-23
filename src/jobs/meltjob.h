/*
 * Copyright (c) 2012-2018 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#ifndef MELTJOB_H
#define MELTJOB_H

#include "abstractjob.h"
#include <QTemporaryFile>

class MeltJob : public AbstractJob
{
    Q_OBJECT
public:
    MeltJob(const QString& name, const QString& xml = QString());
    MeltJob(const QString& name, const QStringList& args);
    virtual ~MeltJob();
    QString xml();
    QString xmlPath() const { return m_xml.fileName(); }
    void setIsStreaming(bool streaming);

public slots:
    void start();
    void onViewXmlTriggered();

protected slots:
    virtual void onOpenTiggered();
    void onShowFolderTriggered();
    
private:
    void onReadyRead();
    QTemporaryFile m_xml;
    bool m_isStreaming;
    int m_previousPercent;
    QStringList m_args;    
};

#endif // MELTJOB_H
