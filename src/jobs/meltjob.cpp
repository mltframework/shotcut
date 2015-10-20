/*
 * Copyright (c) 2012-2015 Meltytech, LLC
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

#include "meltjob.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QIODevice>
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDebug>
#include "mainwindow.h"
#include "dialogs/textviewerdialog.h"

MeltJob::MeltJob(const QString& name, const QString& xml)
    : AbstractJob(name)
    , m_xml(xml)
{
    QAction* action = new QAction(tr("View XML"), this);
    action->setToolTip(tr("View the MLT XML for this job"));
    connect(action, SIGNAL(triggered()), this, SLOT(onViewXmlTriggered()));
    m_standardActions << action;
}

MeltJob::~MeltJob()
{
    qDebug();
    QFile::remove(m_xml);
}

void MeltJob::start()
{
    QString shotcutPath = qApp->applicationDirPath();
#ifdef Q_OS_WIN
    QFileInfo meltPath(shotcutPath, "qmelt.exe");
#else
    QFileInfo meltPath(shotcutPath, "qmelt");
#endif
    setReadChannel(QProcess::StandardError);
    QStringList args;
    args << "-progress2";
    args << "-abort";
    args << m_xml;
    qDebug() << meltPath.absoluteFilePath() << args;
#ifdef Q_OS_WIN
    args << "-getc";
    QProcess::start(meltPath.absoluteFilePath(), args);
#else
    args.prepend(meltPath.absoluteFilePath());
    QProcess::start("/usr/bin/nice", args);
#endif
    AbstractJob::start();
}

QString MeltJob::xml() const
{
    QFile f(m_xml);
    f.open(QIODevice::ReadOnly);
    QString s(f.readAll());
    f.close();
    return s;
}

void MeltJob::onViewXmlTriggered()
{
    TextViewerDialog dialog(&MAIN);
    dialog.setWindowTitle(tr("MLT XML"));
    dialog.setText(xml());
    dialog.exec();
}
