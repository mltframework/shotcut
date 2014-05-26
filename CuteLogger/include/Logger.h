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
#ifndef LOGGER_H
#define LOGGER_H
/**
 * \file Logger.h
 * \brief A file containing the description of Logger class and and additional useful macros for logging
 */

// Qt
#include <QString>
#include <QDebug>
class QDateTime;

// Local
#include "CuteLogger_global.h"
class AbstractAppender;


//! Writes the trace log record
/**
 * This macro is the convinient way to call Logger::write(). It uses the common preprocessor macros \c __FILE__,
 * \c __LINE__ and the standart Qt \c Q_FUNC_INFO macros to automatically determine the needed parameters to call
 * Logger::write().
 *
 * \note This and other (LOG_INFO() etc...) macros uses the variadic macro arguments to give convinient usage form for
 * the different versions of Logger::write() (using the QString or const char* argument or returning the QDebug class
 * instance). Not all compilers will support this. Please, consider reviewing your compiler documentation to ensure
 * it support __VA_ARGS__ macro.
 *
 * It is checked to work with GCC 4.4 or later.
 *
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */
#define LOG_TRACE(...)   Logger::write(Logger::Trace, __FILE__, __LINE__, Q_FUNC_INFO, ##__VA_ARGS__)

//! Writes the debug log record
/**
 * This macro records the info log record using the Logger::write() function. It works identically to the LOG_TRACE()
 * macro.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */
#define LOG_DEBUG(...)   Logger::write(Logger::Debug, __FILE__, __LINE__, Q_FUNC_INFO, ##__VA_ARGS__)

//! Write the info log record
/**
 * This macro records the info log record using the Logger::write() function. It works identically to the LOG_TRACE()
 * macro.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */
#define LOG_INFO(...)    Logger::write(Logger::Info, __FILE__, __LINE__, Q_FUNC_INFO, ##__VA_ARGS__)

//! Write the warning log record
/**
 * This macro records the warning log record using the Logger::write() function. It works identically to the LOG_TRACE()
 * macro.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */
#define LOG_WARNING(...) Logger::write(Logger::Warning, __FILE__, __LINE__, Q_FUNC_INFO, ##__VA_ARGS__)

//! Write the error log record
/**
 * This macro records the error log record using the Logger::write() function. It works identically to the LOG_TRACE()
 * macro.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */
#define LOG_ERROR(...)   Logger::write(Logger::Error, __FILE__, __LINE__, Q_FUNC_INFO, ##__VA_ARGS__)

//! Write the fatal log record
/**
 * This macro records the fatal log record using the Logger::write() function. It works identically to the LOG_TRACE()
 * macro.
 *
 * \note Recording of the log record using the Logger::Fatal log level will lead to calling the STL abort()
 *       function, which will interrupt the running of your software and begin the writing of the core dump.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */
#define LOG_FATAL(...)   Logger::write(Logger::Fatal, __FILE__, __LINE__, Q_FUNC_INFO, ##__VA_ARGS__)

//! Check the assertion
/**
 * This macro is a convinient and recommended to use way to call Logger::writeAssert() function. It uses the
 * preprocessor macros (as the LOG_DEBUG() does) to fill the necessary arguments of the Logger::writeAssert() call. It
 * also uses undocumented but rather mature and stable \c qt_noop() function (which does nothing) when the assertion
 * is true.
 *
 * Example:
 * \code
 * bool b = checkSomething();
 * ...
 * LOG_ASSERT(b == true);
 * \endcode
 *
 * \sa Logger::writeAssert()
 */
#define LOG_ASSERT(cond) ((!(cond)) ? Logger::writeAssert(__FILE__, __LINE__, Q_FUNC_INFO, #cond) : qt_noop())


/**
 * \mainpage
 *
 * Logger is a simple way to write the history of your application lifecycle to any target logging device (which is
 * called Appender and may write to any target you will implement with it: console, text file, XML or something - you
 * choose) and to map logging message to a class, function, source file and line of code which it is called from.
 *
 * Some simple appenders (which may be considered an examples) are provided with the logger itself: see ConsoleAppender
 * and FileAppender documentation.
 *
 * It supports using it in a multithreaded applications, so ALL of its functions are thread safe.
 *
 * Simple usage example:
 * \code
 * #include <QCoreApplication>
 *
 * #include <Logger.h>
 * #include <ConsoleAppender.h>
 *
 * int main(int argc, char* argv[])
 * {
 *   QCoreApplication app(argc, argv);
 *   ...
 *   ConsoleAppender* consoleAppender = new ConsoleAppender();
 *   consoleAppender->setFormat("[%-7l] <%C> %m\n");
 *   Logger::registerAppender(consoleAppender);
 *   ...
 *   LOG_INFO("Starting the application");
 *   int result = app.exec();
 *   ...
 *   if (result)
 *     LOG_WARNING() << "Something went wrong." << "Result code is" << result;
 *
 *   return result;
 * }
 * \endcode
 *
 * Logger internally uses the lazy-initialized singleton object and needs no definite initialization, but you may
 * consider registering a log appender before calling any log recording functions or macros.
 *
 * The library design of Logger allows you to simply mass-replace all occurrences of qDebug and similiar calls with
 * similiar Logger macros (e.g. LOG_DEBUG)
 *
 * \note Logger uses a singleton class which must live through all the application life cycle and cleans it on the
 *       destruction of the QCoreApplication (or QApplication) instance. It needs a QCoreApplication instance to be
 *       created before any of the Logger's functions are called.
 *
 * \sa AbstractAppender
 * \sa LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL
 * \sa LOG_ASSERT
 */

//! Very simple but rather powerful component which may be used for logging your application activities.
class CUTELOGGERSHARED_EXPORT Logger
{
  public:
    //! Describes the possible severity levels of the log records
    enum LogLevel
    {
      Trace,   //!< Trace level. Can be used for mostly unneeded records used for internal code tracing.
      Debug,   //!< Debug level. Useful for non-necessary records used for the debugging of the software.
      Info,    //!< Info level. Can be used for informational records, which may be interesting for not only developers.
      Warning, //!< Warning. May be used to log some non-fatal warnings detected by your application.
      Error,   //!< Error. May be used for a big problems making your application work wrong but not crashing.
      Fatal    //!< Fatal. Used for unrecoverable errors, crashes the application right after the log record is written.
    };

    //! Converts the LogLevel enum value to its string representation
    /**
     * \param logLevel Log level to convert
     *
     * \sa LogLevel
     * \sa levelFromString()
     */
    static QString levelToString(LogLevel logLevel);

    //! Converts the LogLevel string representation to enum value
    /**
     * Comparation of the strings is case independent. If the log level string provided cannot be understood
     * Logger::Debug is returned.
     *
     * \param s String to be decoded
     *
     * \sa LogLevel
     * \sa levelToString()
     */
    static LogLevel levelFromString(const QString& s);

    //! Registers the appender to write the log records to
    /**
     * On the log writing call (using one of the macros or the write() function) Logger traverses through the list of
     * the appenders and writes a log records to the each of them. Please, look through the AbstractAppender
     * documentation to understand the concept of appenders.
     *
     * If no appenders was added to Logger, it falls back to logging into the \c std::cerr STL stream.
     *
     * \param appender Appender to register in the Logger
     *
     * \note Logger takes ownership on the appender and it will delete it on the application exit. According to this,
     *       appenders must be created on heap to prevent double destruction of the appender.
     *
     * \sa AbstractAppender
     */
    static void registerAppender(AbstractAppender* appender);

    //! Writes the log record
    /**
     * Writes the log records with the supplied arguments to all the registered appenders.
     *
     * \note It is not recommended to call this function directly. Instead of this you can just call one of the macros
     *       (LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL) that will supply all the needed
     *       information to this function.
     *
     * \param timeStamp - the time stamp of the record
     * \param logLevel - the log level of the record
     * \param file - the name of the source file that requested the log record
     * \param line - the line of the code of source file that requested the log record
     * \param function - name of the function that requested the log record
     * \param message - log message
     *
     * \note Recording of the log record using the Logger::Fatal log level will lead to calling the STL abort()
     *       function, which will interrupt the running of your software and begin the writing of the core dump.
     *
     * \sa LogLevel
     * \sa LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL
     * \sa AbstractAppender
     */
    static void write(const QDateTime& timeStamp, LogLevel logLevel, const char* file, int line, const char* function,
                      const QString& message);

    /**
     * This is the overloaded function provided for the convinience. It behaves identically to the above function.
     *
     * This function uses the current timestamp obtained with \c QDateTime::currentDateTime().
     *
     * \sa write()
     */
    static void write(LogLevel logLevel, const char* file, int line, const char* function, const QString& message);

    /**
     * This is the overloaded function provided for the convinience. It behaves identically to the above function.
     *
     * This function uses the current timestamp obtained with \c QDateTime::currentDateTime(). Also it supports writing
     * <tt>const char*</tt> instead of \c QString and converts it internally using the \c QString::fromAscii(). If you
     * want this function to support the non-ascii strings, you will need to setup the codec using the
     * \c QTextCodec::setCodecForCStrings()
     *
     * \sa write()
     */
    static void write(LogLevel logLevel, const char* file, int line, const char* function, const char* message, ...);

    /**
     * This is the overloaded function provided for the convinience. It behaves identically to the above function.
     *
     * This function doesn't accept any log message as argument. It returns the \c QDebug object that can be written
     * using the stream functions. For example, you may like to write:
     * \code
     * LOG_DEBUG() << "This is the size" << size << "of the element" << elementName;
     * \endcode
     * instead of writing
     * \code
     * LOG_DEBUG(QString(QLatin1String("This is the size %1x%2 of the element %3"))
     *           .arg(size.x()).arg(size.y()).arg(elementName));
     * \endcode
     *
     * Please consider reading the Qt Reference Documentation for the description of the QDebug class usage syntax.
     *
     * \note This overload is definitely more pleasant to use than the first write() overload, but it behaves definitely
     *       slower than all the above overloads.
     *
     * \sa write()
     */
    static QDebug write(LogLevel logLevel, const char* file, int line, const char* function);

    //! Writes the assertion
    /**
     * This function writes the assertion record using the write() function.
     *
     * The assertion record is always written using the Logger::Fatal log level which leads to the abortation of the
     * program and generation of the core dump (if supported).
     *
     * The message written to the appenders will be identical to the \c condition argument prefixed with the
     * <tt>ASSERT:</tt> notification.
     *
     * \note It is not recommended to call this function directly. Instead of this you can just call the LOG_ASSERT
     *       macro that will supply all the needed information to this function.
     *
     * \sa LOG_ASSERT
     * \sa write()
     */
    static void writeAssert(const char* file, int line, const char* function, const char* condition);
};

#endif // LOGGER_H
