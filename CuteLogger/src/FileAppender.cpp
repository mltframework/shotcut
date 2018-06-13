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

/**
 * \class FileAppender
 *
 * \brief Simple appender that writes the log records to the plain text file.
 */


//! Constructs the new file appender assigned to file with the given name.
FileAppender::FileAppender(const QString& fileName)
{
  setFileName(fileName);
}


FileAppender::~FileAppender()
{
  closeFile();
}


//! Returns the name set by setFileName() or to the FileAppender constructor.
/**
 * \sa setFileName()
 */
QString FileAppender::fileName() const
{
  QMutexLocker locker(&m_logFileMutex);
  return m_logFile.fileName();
}


//! Sets the name of the file. The name can have no path, a relative path, or an absolute path.
/**
 * \sa fileName()
 */
void FileAppender::setFileName(const QString& s)
{
  QMutexLocker locker(&m_logFileMutex);
  if (m_logFile.isOpen())
    m_logFile.close();

  m_logFile.setFileName(s);
}


bool FileAppender::openFile()
{
  bool isOpen = m_logFile.isOpen();
  if (!isOpen)
  {
    isOpen = m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    if (isOpen)
      m_logStream.setDevice(&m_logFile);
    else
      std::cerr << "<FileAppender::append> Cannot open the log file " << qPrintable(m_logFile.fileName()) << std::endl;
  }
  return isOpen;
}


//! Write the log record to the file.
/**
 * \sa fileName()
 * \sa AbstractStringAppender::format()
 */
void FileAppender::append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                          const char* function, const QString& category, const QString& message)
{
  QMutexLocker locker(&m_logFileMutex);

  if (openFile())
  {
    m_logStream << formattedString(timeStamp, logLevel, file, line, function, category, message);
    m_logStream.flush();
    m_logFile.flush();
  }
}


void FileAppender::closeFile()
{
  QMutexLocker locker(&m_logFileMutex);
  m_logFile.close();
}
