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
#ifndef ABSTRACTSTRINGAPPENDER_H
#define ABSTRACTSTRINGAPPENDER_H

// Local
#include "CuteLogger_global.h"
#include <AbstractAppender.h>

// Qt
#include <QReadWriteLock>


//! The AbstractStringAppender class provides a convinient base for appenders working with plain text formatted logs.
/**
 * AbstractSringAppender is the simple extension of the AbstractAppender class providing the convinient way to create
 * custom log appenders working with a plain text formatted log targets.
 *
 * It have the formattedString() protected function that formats the logging arguments according to a format set with
 * setFormat().
 *
 * This class can not be directly instantiated because it contains pure virtual function inherited from AbstractAppender
 * class.
 *
 * For more detailed description of customizing the log output format see the documentation on the setFormat() function.
 */
class CUTELOGGERSHARED_EXPORT AbstractStringAppender : public AbstractAppender
{
  public:
    //! Constructs a new string appender object
    AbstractStringAppender();

    //! Returns the current log format string.
    /**
     * The default format is set to "%t{yyyy-MM-ddTHH:mm:ss.zzz} [%-7l] <%C> %m\n". You can set a different log record
     * format using the setFormat() function.
     *
     * \sa setFormat(const QString&)
     */
    QString format() const;

    //! Sets the logging format for writing strings to the log target with this appender.
    /**
     * The string format seems to be very common to those developers who have used a standart sprintf function.
     *
     * Log output format is a simple QString with the special markers (starting with % sign) which will be replaced with
     * it's internal meaning when writing a log record.
     *
     * Controlling marker begins with the percent sign (%) which is followed by (optional) field width argument, the
     * (necessary) single-letter command (which describes, what will be put to log record instead of marker, and an
     * additional formatting argument (in the {} brackets) supported for some of the log commands.
     *
     * Field width argument works almost identically to the \c QString::arg() \c fieldWidth argument (and uses it
     * internally). For example, \c "%-7l" will be replaced with the left padded debug level of the message
     * (\c "Debug  ") or something. For the more detailed description of it you may consider to look to the Qt
     * Reference Documentation.
     *
     * Supported marker commands are:
     *   \arg \c %t - timestamp. You may specify your custom timestamp format using the {} brackets after the marker,
     *           timestamp format here will be similiar to those used in QDateTime::toString() function. For example,
     *           "%t{dd-MM-yyyy, HH:mm}" may be replaced with "17-12-2010, 20:17" depending on current date and time.
     *           The default format used here is "HH:mm:ss.zzz".
     *   \arg \c %l - Log level. Possible log levels are shown in the Logger::LogLevel enumerator.
     *   \arg \c %L - Uppercased log level.
     *   \arg \c %F - Full source file name (with path) of the file that requested log recording. Uses the \c __FILE__
     *           preprocessor macro.
     *   \arg \c %f - Short file name (with stripped path).
     *   \arg \c %i - Line number in the source file. Uses the \c __LINE__ preprocessor macro.
     *   \arg \c %C - Name of function that called on of the LOG_* macros. Uses the \c Q_FUNC_INFO macro provided with
     *           Qt.
     *   \arg \c %c - [EXPERIMENTAL] Similiar to the %C, but the function name is stripped using stripFunctionName
     *   \arg \c %m - The log message sent by the caller.
     *   \arg \c %% - Convinient marker that is replaced with the single \c % mark.
     *
     * \note Format doesn't add \c '\\n' to the end of the format line. Please consider adding it manually.
     *
     * \sa format()
     * \sa stripFunctionName()
     * \sa Logger::LogLevel
     */
    void setFormat(const QString&);

    //! Strips the long function signature (as added by Q_FUNC_INFO macro)
    /**
     * The string processing drops the returning type, arguments and template parameters of function. It is definitely
     * useful for enchancing the log output readability.
     * \return stripped function name
     */
    static QString stripFunctionName(const char*);

  protected:
    //! Returns the string to record to the logging target, formatted according to the format().
    /**
     * \sa format()
     * \sa setFormat(const QString&)
     */
    QString formattedString(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                            const char* function, const QString& message) const;

  private:
    QString m_format;
    mutable QReadWriteLock m_formatLock;
};

#endif // ABSTRACTSTRINGAPPENDER_H
