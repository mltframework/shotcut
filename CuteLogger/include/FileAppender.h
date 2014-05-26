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
#ifndef FILEAPPENDER_H
#define FILEAPPENDER_H

// Logger
#include "CuteLogger_global.h"
#include <AbstractStringAppender.h>

// Qt
#include <QFile>
#include <QTextStream>


//! File is the simple appender that writes the log records to the plain text file.
class CUTELOGGERSHARED_EXPORT FileAppender : public AbstractStringAppender
{
  public:
    //! Constructs the new file appender assigned to file with the given name.
    FileAppender(const QString& fileName = QString());
    ~FileAppender();

    //! Returns the name set by setFileName() or to the FileAppender constructor.
    /**
     * \sa setFileName()
     */
    QString fileName() const;

    //! Sets the name of the file. The name can have no path, a relative path, or an absolute path.
    /**
     * \sa fileName()
     */
    void setFileName(const QString&);

  protected:
    //! Write the log record to the file.
    /**
     * \sa fileName()
     * \sa AbstractStringAppender::format()
     */
    virtual void append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                        const char* function, const QString& message);
    bool openFile();
    void closeFile();

  private:
    QFile m_logFile;
    QTextStream m_logStream;
    mutable QMutex m_logFileMutex;
};

#endif // FILEAPPENDER_H
