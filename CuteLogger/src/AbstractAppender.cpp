/*
  Copyright (c) 2010 Boris Moiseev (cyberbobs at gmail dot com)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1
  as published by the Free Software Foundation and appearing in the file
  LICENSE.LGPL included in the packaging of this file.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.
*/
// Local
#include "AbstractAppender.h"

// Qt
#include <QMutexLocker>


AbstractAppender::AbstractAppender()
  : m_detailsLevel(Logger::Debug)
{}


AbstractAppender::~AbstractAppender()
{}


Logger::LogLevel AbstractAppender::detailsLevel() const
{
  QMutexLocker locker(&m_detailsLevelMutex);
  return m_detailsLevel;
}


void AbstractAppender::setDetailsLevel(Logger::LogLevel level)
{
  QMutexLocker locker(&m_detailsLevelMutex);
  m_detailsLevel = level;
}


void AbstractAppender::setDetailsLevel(const QString& level)
{
  setDetailsLevel(Logger::levelFromString(level));
}


void AbstractAppender::write(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                             const char* function, const QString& message)
{
  if (logLevel >= detailsLevel())
  {
    QMutexLocker locker(&m_writeMutex);
    append(timeStamp, logLevel, file, line, function, message);
  }
}
