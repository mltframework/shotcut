/*
  Copyright (c) 2010 Karl-Heinz Reichel (khreichel at googlemail dot com)

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
#include "OutputDebugAppender.h"

// STL
#include <windows.h>


/**
 * \class OutputDebugAppender
 *
 * \brief Appender that writes the log records to the Microsoft Debug Log
 */


//! Writes the log record to the windows debug log.
/**
 * \sa AbstractStringAppender::format()
 */
void OutputDebugAppender::append(const QDateTime& timeStamp,
                                 Logger::LogLevel logLevel,
                                 const char* file,
                                 int line,
                                 const char* function,
                                 const QString& category,
                                 const QString& message)
{
    QString s = formattedString(timeStamp, logLevel, file, line, function, category, message);
    OutputDebugStringW((LPCWSTR) s.utf16());
}

