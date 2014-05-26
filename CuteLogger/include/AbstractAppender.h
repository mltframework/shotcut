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
#ifndef ABSTRACTAPPENDER_H
#define ABSTRACTAPPENDER_H

// Local
#include "CuteLogger_global.h"
#include <Logger.h>

// Qt
#include <QMutex>

//! The AbstractAppender class provides an abstract base class for writing a log entries.
/**
 * The AbstractAppender class is the base interface class for all log appenders that could be used with Logger.
 *
 * AbstractAppender provides a common implementation for the thread safe, mutex-protected logging of application
 * messages, such as ConsoleAppender, FileAppender or something else. AbstractAppender is abstract and can not be
 * instantiated, but you can use any of its subclasses or create a custom log appender at your choice.
 *
 * Appenders are the logical devices that is aimed to be attached to Logger object by calling
 * Logger::registerAppender(). On each log record call from the application Logger object sequentially calls write()
 * function on all the appenders registered in it.
 *
 * You can subclass AbstractAppender to implement a logging target of any kind you like. It may be the external logging
 * subsystem (for example, syslog in *nix), XML file, SQL database entries, D-Bus messages or anything else you can
 * imagine.
 *
 * For the simple non-structured plain text logging (for example, to a plain text file or to the console output) you may
 * like to subclass the AbstractStringAppender instead of AbstractAppender, which will give you a more convinient way to
 * control the format of the log output.
 *
 * \sa AbstractStringAppender
 * \sa Logger::registerAppender()
 */
class CUTELOGGERSHARED_EXPORT AbstractAppender
{
  public:
    //! Constructs a AbstractAppender object.
    AbstractAppender();

    //! Destructs the AbstractAppender object.
    virtual ~AbstractAppender();

    //! Returns the current details level of appender.
    /**
     * Log records with a log level lower than a current detailsLevel() will be silently ignored by appender and would not
     * be sent to its append() function.
     *
     * It provides additional logging flexibility, allowing you to set the different severity levels for different types
     * of logs.
     *
     * \note This function is thread safe.
     *
     * \sa setDetailsLevel()
     * \sa Logger::LogLevel
     */
    Logger::LogLevel detailsLevel() const;

    //! Sets the current details level of appender.
    /**
     * \note This function is thread safe.
     *
     * \sa detalsLevel()
     * \sa Logger::LogLevel
     */
    void setDetailsLevel(Logger::LogLevel level);

    //! Sets the current details level of appender
    /**
     * This function is provided for convinience, it behaves like an above function.
     *
     * \sa detalsLevel()
     * \sa Logger::LogLevel
     */
    void setDetailsLevel(const QString& level);

    //! Tries to write the log record to this logger
    /**
     * This is the function called by Logger object to write a log message to the appender.
     *
     * \note This function is thread safe.
     *
     * \sa Logger::write()
     * \sa detailsLevel()
     */
    void write(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line, const char* function,
               const QString& message);

  protected:
    //! Writes the log record to the logger instance
    /**
     * This function is called every time when user tries to write a message to this AbstractAppender instance using
     * the write() function. Write function works as proxy and transfers only the messages with log level more or equal
     * to the current logLevel().
     *
     * Overload this function when you are implementing a custom appender.
     *
     * \note This function is not needed to be thread safe because it is never called directly by Logger object. The
     * write() function works as a proxy and protects this function from concurrent access.
     *
     * \sa Logger::write()
     */
    virtual void append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                        const char* function, const QString& message) = 0;

  private:
    QMutex m_writeMutex;

    Logger::LogLevel m_detailsLevel;
    mutable QMutex m_detailsLevelMutex;
};

#endif // ABSTRACTAPPENDER_H
