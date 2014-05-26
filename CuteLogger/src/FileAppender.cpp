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
#include "FileAppender.h"

// STL
#include <iostream>


FileAppender::FileAppender(const QString& fileName)
{
  setFileName(fileName);
}


FileAppender::~FileAppender()
{
  closeFile();
}


QString FileAppender::fileName() const
{
  QMutexLocker locker(&m_logFileMutex);
  return m_logFile.fileName();
}


void FileAppender::setFileName(const QString& s)
{
  QMutexLocker locker(&m_logFileMutex);
  if (m_logFile.isOpen())
    m_logFile.close();

  m_logFile.setFileName(s);
}


bool FileAppender::openFile()
{
  bool isOpen = false;
  if (!m_logFile.isOpen())
  {
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
      m_logStream.setDevice(&m_logFile);
      isOpen = true;
    }
    else
    {
      std::cerr << "<FileAppender::append> Cannot open the log file " << qPrintable(m_logFile.fileName()) << std::endl;
    }
  }
  return isOpen;
}


void FileAppender::append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                          const char* function, const QString& message)
{
  QMutexLocker locker(&m_logFileMutex);

  openFile();

  m_logStream << formattedString(timeStamp, logLevel, file, line, function, message);
  m_logStream.flush();
  m_logFile.flush();
}

void FileAppender::closeFile()
{
  QMutexLocker locker(&m_logFileMutex);
  m_logFile.close();
}
