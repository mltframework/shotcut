/*
  Copyright (c) 2012 Boris Moiseev (cyberbobs at gmail dot com)

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
#include "Logger.h"
#include "AbstractAppender.h"
#include "AbstractStringAppender.h"

// Qt
#include <QCoreApplication>
#include <QReadWriteLock>
#include <QSemaphore>
#include <QDateTime>
#include <QIODevice>

#if defined(Q_OS_ANDROID)
#  include <android/log.h>
#  include <AndroidAppender.h>
#endif

// STL
#include <iostream>


/**
 * \file Logger.h
 * \brief A file containing the description of Logger class and and additional useful macros for logging
 */


/**
 * \mainpage
 *
 * Logger is a simple way to write the history of your application lifecycle to any target logging device (which is
 * called Appender and may write to any target you will implement with it: console, text file, XML or something - you
 * choose) and to map logging message to a class, function, source file and line of code which it is called from.
 *
 * Some simple appenders (which may be considered an examples) are provided with the Logger itself: see ConsoleAppender
 * and FileAppender documentation.
 *
 * It supports using it in a multithreaded applications, so all of its functions are thread safe.
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
 *   ConsoleAppender* consoleAppender = new ConsoleAppender;
 *   consoleAppender->setFormat("[%{type:-7}] <%{Function}> %{message}\n");
 *   cuteLogger->registerAppender(consoleAppender);
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
 * The library design of Logger allows you to simply mass-replace all occurrences of qDebug and similar calls with
 * similar Logger macros (e.g. LOG_DEBUG())
 *
 * \note Logger uses a singleton global instance which lives through all the application life cycle and self-destroys
 *       destruction of the QCoreApplication (or QApplication) instance. It needs a QCoreApplication instance to be
 *       created before any of the Logger's functions are called.
 *
 * \sa cuteLogger
 * \sa LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL
 * \sa LOG_CTRACE, LOG_CDEBUG, LOG_CINFO, LOG_CWARNING, LOG_CERROR, LOG_CFATAL
 * \sa LOG_ASSERT
 * \sa LOG_TRACE_TIME, LOG_DEBUG_TIME, LOG_INFO_TIME
 * \sa AbstractAppender
 */


/**
 * \def cuteLogger
 *
 * \brief Macro returning the current instance of Logger object
 *
 * If you haven't created a local Logger object it returns the same value as the Logger::globalInstance() functions.
 * This macro is a recommended way to get an access to the Logger instance used in current class.
 *
 * Example:
 * \code
 * ConsoleAppender* consoleAppender = new ConsoleAppender;
 * cuteLogger->registerAppender(consoleAppender);
 * \endcode
 *
 * \sa Logger::globalInstance()
 */


/**
 * \def LOG_TRACE
 *
 * \brief Writes the trace log record
 *
 * This macro is the convinient way to call Logger::write(). It uses the common preprocessor macros \c __FILE__,
 * \c __LINE__ and the standart Qt \c Q_FUNC_INFO macros to automatically determine the needed parameters to call
 * Logger::write().
 *
 * \note This and other (LOG_INFO() etc...) macros uses the variadic macro arguments to give convinient usage form for
 * the different versions of Logger::write() (using the QString or const char* argument or returning the QDebug class
 * instance). Not all compilers will support this. Please, consider reviewing your compiler documentation to ensure
 * it support __VA_ARGS__ macro.
 *
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */


/**
 * \def LOG_DEBUG
 *
 * \brief Writes the debug log record
 *
 * This macro records the debug log record using the Logger::write() function. It works similar to the LOG_TRACE()
 * macro.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */


/**
 * \def LOG_INFO
 *
 * \brief Writes the info log record
 *
 * This macro records the info log record using the Logger::write() function. It works similar to the LOG_TRACE()
 * macro.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */


/**
 * \def LOG_WARNING
 *
 * \brief Write the warning log record
 *
 * This macro records the warning log record using the Logger::write() function. It works similar to the LOG_TRACE()
 * macro.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */


/**
 * \def LOG_ERROR
 *
 * \brief Write the error log record
 * This macro records the error log record using the Logger::write() function. It works similar to the LOG_TRACE()
 * macro.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */


/**
 * \def LOG_FATAL
 *
 * \brief Write the fatal log record
 *
 * This macro records the fatal log record using the Logger::write() function. It works similar to the LOG_TRACE()
 * macro.
 *
 * \note Recording of the log record using the Logger::Fatal log level will lead to calling the STL abort()
 *       function, which will interrupt the running of your software and begin the writing of the core dump.
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::write()
 */


/**
 * \def LOG_CTRACE(category)
 *
 * \brief Writes the trace log record to the specific category
 *
 * This macro is the similar to the LOG_TRACE() macro, but has a category parameter
 * to write only to the category appenders (registered using Logger::registerCategoryAppender() method).
 *
 * \param category category name string
 *
 * \sa LOG_TRACE()
 * \sa Logger::LogLevel
 * \sa Logger::registerCategoryAppender()
 * \sa Logger::write()
 * \sa LOG_CATEGORY(), LOG_GLOBAL_CATEGORY()
 */


/**
 * \def LOG_CDEBUG
 *
 * \brief Writes the debug log record to the specific category
 *
 * This macro records the debug log record using the Logger::write() function. It works similar to the LOG_CTRACE()
 * macro.
 *
 * \sa LOG_CTRACE()
 */


/**
 * \def LOG_CINFO
 *
 * \brief Writes the info log record to the specific category
 *
 * This macro records the info log record using the Logger::write() function. It works similar to the LOG_CTRACE()
 * macro.
 *
 * \sa LOG_CTRACE()
 */


/**
 * \def LOG_CWARNING
 *
 * \brief Writes the warning log record to the specific category
 *
 * This macro records the warning log record using the Logger::write() function. It works similar to the LOG_CTRACE()
 * macro.
 *
 * \sa LOG_CTRACE()
 */


/**
 * \def LOG_CERROR
 *
 * \brief Writes the error log record to the specific category
 *
 * This macro records the error log record using the Logger::write() function. It works similar to the LOG_CTRACE()
 * macro.
 *
 * \sa LOG_CTRACE()
 */


/**
 * \def LOG_CFATAL
 *
 * \brief Write the fatal log record to the specific category
 *
 * This macro records the fatal log record using the Logger::write() function. It works similar to the LOG_CTRACE()
 * macro.
 *
 * \note Recording of the log record using the Logger::Fatal log level will lead to calling the STL abort()
 *       function, which will interrupt the running of your software and begin the writing of the core dump.
 *
 * \sa LOG_CTRACE()
 */


/**
 * \def LOG_CATEGORY(category)
 *
 * \brief Create logger instance inside your custom class to log all messages to the specified category
 *
 * This macro is used to pass all log messages inside your custom class to the specific category.
 * You must include this macro inside your class declaration (similarly to the Q_OBJECT macro).
 * Internally, this macro redefines cuteLoggerInstance() function, creates the local Logger object inside your class and
 * sets the default category to the specified parameter.
 *
 * Thus, any call to cuteLoggerInstance() (for example, inside LOG_TRACE() macro) will return the local Logger object,
 * so any logging message will be directed to the default category.
 *
 * \note This macro does not register any appender to the newly created logger instance. You should register
 * logger appenders manually, inside your class.
 *
 * Usage example:
 * \code
 * class CustomClass : public QObject
 * {
 *   Q_OBJECT
 *   LOG_CATEGORY("custom_category")
 *   ...
 * };
 *
 * CustomClass::CustomClass(QObject* parent) : QObject(parent)
 * {
 *   cuteLogger->registerAppender(new FileAppender("custom_category_log"));
 *   LOG_TRACE() << "Trace to the custom category log";
 * }
 * \endcode
 *
 * If used compiler supports C++11 standard, LOG_CATEGORY and LOG_GLOBAL_CATEGORY macros would also work when added
 * inside of any scope. It could be useful, for example, to log every single run of a method to a different file.
 *
 * \code
 * void foo()
 * {
 *   QString categoryName = QDateTime::currentDateTime().toString("yyyy-MM-ddThh-mm-ss-zzz");
 *   LOG_CATEGORY(categoryName);
 *   cuteLogger->registerAppender(new FileAppender(categoryName + ".log"));
 *   ...
 * }
 * \endcode
 *
 * \sa Logger::write()
 * \sa LOG_TRACE
 * \sa Logger::registerCategoryAppender()
 * \sa Logger::setDefaultCategory()
 * \sa LOG_GLOBAL_CATEGORY
 */


/**
 * \def LOG_GLOBAL_CATEGORY(category)
 *
 * \brief Create logger instance inside your custom class to log all messages both to the specified category and to
 * the global logger instance.
 *
 * This macro is similar to LOG_CATEGORY(), but also passes all log messages to the global logger instance appenders.
 * It is equal to defining the local category logger using LOG_CATEGORY macro and calling:
 * \code cuteLogger->logToGlobalInstance(cuteLogger->defaultCategory(), true); \endcode
 *
 * \sa LOG_CATEGORY
 * \sa Logger::logToGlobalInstance()
 * \sa Logger::defaultCategory()
 * \sa Logger::registerCategoryAppender()
 * \sa Logger::write()
 */



/**
 * \def LOG_ASSERT
 *
 * \brief Check the assertion
 *
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


/**
 * \def LOG_TRACE_TIME
 *
 * \brief Logs the processing time of current function / code block
 *
 * This macro automagically measures the function or code of block execution time and outputs it as a Logger::Trace
 * level log record.
 *
 * Example:
 * \code
 * int foo()
 * {
 *   LOG_TRACE_TIME();
 *   ... // Do some long operations
 *   return 0;
 * } // Outputs: Function foo finished in <time> ms.
 * \endcode
 *
 * If you are measuring a code of block execution time you may also add a name of block to the macro:
 * \code
 * int bar(bool doFoo)
 * {
 *   LOG_TRACE_TIME();
 *
 *   if (doFoo)
 *   {
 *     LOG_TRACE_TIME("Foo");
 *     ...
 *   }
 *
 *   ...
 * }
 * // Outputs:
 * // "Foo" finished in <time1> ms.
 * // Function bar finished in <time2> ms.
 * \endcode
 *
 * \note Macro switches to logging the seconds instead of milliseconds when the execution time reaches 10000 ms.
 * \sa LOG_DEBUG_TIME, LOG_INFO_TIME
 */


/**
 * \def LOG_DEBUG_TIME
 *
 * \brief Logs the processing time of current function / code block
 *
 * This macro automagically measures the function or code of block execution time and outputs it as a Logger::Debug
 * level log record. It works similar to LOG_TRACE_TIME() macro.
 *
 * \sa LOG_TRACE_TIME
 */


/**
 * \def LOG_INFO_TIME
 *
 * \brief Logs the processing time of current function / code block
 *
 * This macro automagically measures the function or code of block execution time and outputs it as a Logger::Info
 * level log record. It works similar to LOG_TRACE_TIME() macro.
 *
 * \sa LOG_TRACE_TIME
 */


/**
 * \class Logger
 *
 * \brief Very simple but rather powerful component which may be used for logging your application activities.
 *
 * Global logger instance created on a first access to it (e.g. registering appenders, calling a LOG_DEBUG() macro
 * etc.) registers itself as a Qt default message handler and captures all the qDebug/qWarning/qCritical output.
 *
 * \note Qt 4 qDebug set of macro doesn't support capturing source function name, file name or line number so we
 *       recommend to use LOG_DEBUG() and other Logger macros instead.
 *
 * \sa cuteLogger
 * \sa [CuteLogger Documentation](index.html)
 */

// Forward declarations
static void cleanupLoggerGlobalInstance();

#if QT_VERSION >= 0x050000
static void qtLoggerMessageHandler(QtMsgType, const QMessageLogContext& context, const QString& msg);
#else
static void qtLoggerMessageHandler(QtMsgType type, const char* msg);
#endif

/**
 * \internal
 *
 * LoggerPrivate class implements the Singleton pattern in a thread-safe way. It contains a static pointer to the
 * global logger instance protected by QReadWriteLock
 */
class LoggerPrivate
{
  public:
    static Logger* globalInstance;
    static QReadWriteLock globalInstanceLock;

    QList<AbstractAppender*> appenders;
    QMutex loggerMutex;

    QMap<QString, bool> categories;
    QMultiMap<QString, AbstractAppender*> categoryAppenders;
    QStringList noAppendersCategories; //<! Categories without appenders that was already warned about
    QString defaultCategory;
    bool writeDefaultCategoryToGlobalInstance;
};


// Static fields initialization
Logger* LoggerPrivate::globalInstance = nullptr;
QReadWriteLock LoggerPrivate::globalInstanceLock;


static void cleanupLoggerGlobalInstance()
{
  QWriteLocker locker(&LoggerPrivate::globalInstanceLock);

  delete LoggerPrivate::globalInstance;
  LoggerPrivate::globalInstance = nullptr;
}


#if QT_VERSION >= 0x050000
static void qtLoggerMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  Logger::LogLevel level = Logger::Debug;
  switch (type)
  {
    case QtDebugMsg:
      level = Logger::Debug;
      break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
      level = Logger::Info;
      break;
#endif
    case QtWarningMsg:
      level = Logger::Warning;
      break;
    case QtCriticalMsg:
      level = Logger::Error;
      break;
    case QtFatalMsg:
      level = Logger::Fatal;
      break;
  }

  bool isDefaultCategory = QString::fromLatin1(context.category) == "default";
  Logger::globalInstance()->write(level, context.file, context.line, context.function, isDefaultCategory ? nullptr : context.category, msg);
}

#else

static void qtLoggerMessageHandler(QtMsgType type, const char* msg)
{
  switch (type)
  {
    case QtDebugMsg:
      cuteLoggerInstance()->write(Logger::Debug, "", 0, "qDebug", 0, msg);
      break;
    case QtWarningMsg:
      cuteLoggerInstance()->write(Logger::Warning, "", 0, "qDebug", 0, msg);
      break;
    case QtCriticalMsg:
      cuteLoggerInstance()->write(Logger::Error, "", 0, "qDebug", 0, msg);
      break;
    case QtFatalMsg:
      cuteLoggerInstance()->write(Logger::Fatal, "", 0, "qDebug", 0, msg);
      break;
  }
}
#endif


//! Construct the instance of Logger
/**
 * If you're only using one global instance of logger you wouldn't probably need to use this constructor manually.
 * Consider using [cuteLogger](@ref cuteLogger) macro instead to access the logger instance
 */
Logger::Logger()
  : d_ptr(new LoggerPrivate)
{
  Q_D(Logger);
  d->writeDefaultCategoryToGlobalInstance = false;
}


//! Construct the instance of Logger and set logger default category
/**
 * If you're only using one global instance of logger you wouldn't probably need to use this constructor manually.
 * Consider using [cuteLogger](@ref cuteLogger) macro instead to access the logger instance and call
 * [setDefaultCategory](@ref setDefaultCategory) method.
 *
 * \sa Logger()
 * \sa setDefaultCategory()
 */
Logger::Logger(const QString& defaultCategory, bool writeToGlobalInstance)
  : d_ptr(new LoggerPrivate)
{
  Q_D(Logger);
  d->writeDefaultCategoryToGlobalInstance = writeToGlobalInstance;

  setDefaultCategory(defaultCategory);
}


//! Destroy the instance of Logger
/**
 * You probably wouldn't need to use this function directly. Global instance of logger will be destroyed automatically
 * at the end of your QCoreApplication execution
 */
Logger::~Logger()
{
  Q_D(Logger);

  // Cleanup appenders
  QMutexLocker appendersLocker(&d->loggerMutex);
#if QT_VERSION >= 0x050e00
  QSet<AbstractAppender*> deleteList(d->appenders.begin(), d->appenders.end());
  auto cal = d->categoryAppenders.values();
  deleteList.unite(QSet<AbstractAppender*>(cal.begin(), cal.end()));
#else
  QSet<AbstractAppender*> deleteList(QSet<AbstractAppender*>::fromList(d->appenders));
  deleteList.unite(QSet<AbstractAppender*>::fromList(d->categoryAppenders.values()));
#endif
  qDeleteAll(deleteList);

  appendersLocker.unlock();

  delete d_ptr;
}


//! Converts the LogLevel enum value to its string representation
/**
 * \param logLevel Log level to convert
 *
 * \sa LogLevel
 * \sa levelFromString()
 */
QString Logger::levelToString(Logger::LogLevel logLevel)
{
  switch (logLevel)
  {
    case Trace:
      return QLatin1String("Trace");
    case Debug:
      return QLatin1String("Debug");
    case Info:
      return QLatin1String("Info");
    case Warning:
      return QLatin1String("Warning");
    case Error:
      return QLatin1String("Error");
    case Fatal:
      return QLatin1String("Fatal");
  }

  return QString();
}


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
Logger::LogLevel Logger::levelFromString(const QString& s)
{
  QString str = s.trimmed().toLower();

  LogLevel result = Debug;

  if (str == QLatin1String("trace"))
    result = Trace;
  else if (str == QLatin1String("debug"))
    result = Debug;
  else if (str == QLatin1String("info"))
    result = Info;
  else if (str == QLatin1String("warning"))
    result = Warning;
  else if (str == QLatin1String("error"))
    result = Error;
  else if (str == QLatin1String("fatal"))
    result = Fatal;

  return result;
}


//! Returns the global instance of Logger
/**
 * In a most cases you shouldn't use this function directly. Consider using [cuteLogger](@ref cuteLogger) macro instead.
 *
 * \sa cuteLogger
 */
Logger* Logger::globalInstance()
{
  Logger* result = nullptr;
  {
    QReadLocker locker(&LoggerPrivate::globalInstanceLock);
    result = LoggerPrivate::globalInstance;
  }

  if (!result)
  {
    QWriteLocker locker(&LoggerPrivate::globalInstanceLock);
    LoggerPrivate::globalInstance = new Logger;

#if QT_VERSION >= 0x050000
    qInstallMessageHandler(qtLoggerMessageHandler);
#else
    qInstallMsgHandler(qtLoggerMessageHandler);
#endif
    qAddPostRoutine(cleanupLoggerGlobalInstance);
    result = LoggerPrivate::globalInstance;
  }

  return result;
}


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
 * \sa registerCategoryAppender
 * \sa AbstractAppender
 */
void Logger::registerAppender(AbstractAppender* appender)
{
  Q_D(Logger);

  QMutexLocker locker(&d->loggerMutex);

  if (!d->appenders.contains(appender))
    d->appenders.append(appender);
  else
    std::cerr << "Trying to register appender that was already registered" << std::endl;
}

//! Registers the appender to write the log records to the specific category
/**
 * Calling this method, you can link some appender with the named category.
 * On the log writing call to the specific category (calling write() with category parameter directly,
 * writing to the default category, or using special LOG_CDEBUG(), LOG_CWARNING() etc. macros),
 * Logger writes the log message only to the list of registered category appenders.
 *
 * You can call logToGlobalInstance() to pass all category log messages to the global logger instance appenders
 * (registered using registerAppender()).
 * If no category appenders with specific name was registered to the Logger,
 * it falls back to logging into the \c std::cerr STL stream, both with simple warning message.
 *
 * \param category Category name
 * \param appender Appender to register in the Logger
 *
 * \note Logger takes ownership on the appender and it will delete it on the application exit. According to this,
 *       appenders must be created on heap to prevent double destruction of the appender.
 *
 * \sa registerAppender
 * \sa LOG_CTRACE(), LOG_CDEBUG(), LOG_CINFO(), LOG_CWARNING(), LOG_CERROR(), LOG_CFATAL()
 * \sa LOG_CATEGORY(), LOG_GLOBAL_CATEGORY()
 * \sa logToGlobalInstance
 * \sa setDefaultCategory
 */
void Logger::registerCategoryAppender(const QString& category, AbstractAppender* appender)
{
  Q_D(Logger);

  QMutexLocker locker(&d->loggerMutex);

  if (!d->categoryAppenders.values().contains(appender))
    d->categoryAppenders.insert(category, appender);
  else
    std::cerr << "Trying to register appender that was already registered" << std::endl;
}


//! Removes the registered appender from logger
/**
 * After calling this function logger stops writing any of the records to the appender.
 *
 * \param appender Pointer to appender to remove from logger
 * \note Removed appender will not be deleted on the application shutdown and you will need to destroy the object
 *       yourself.
 * \sa registerAppender
 */
void Logger::removeAppender(AbstractAppender* appender)
{
  Q_D(Logger);

  QMutexLocker locker(&d->loggerMutex);

  d->appenders.removeAll(appender);
  for (QMultiMap<QString,AbstractAppender*>::iterator it = d->categoryAppenders.begin(); it != d->categoryAppenders.end();)
  {
    if (it.value() == appender)
      it = d->categoryAppenders.erase(it);
    else
      ++it;
  }
}


//! Sets default logging category
/**
 * All log messages to this category appenders will also be written to general logger instance appenders (registered
 * using [registerAppender](@ref registerAppender) method), and vice versa.
 * In particular, any calls to the LOG_DEBUG() macro will be treated as category logging,
 * so you needn't to specify category name using LOG_CDEBUG() macro.
 *
 * To unset the default category, pass a null string as a parameter.
 *
 * \param category Category name
 *
 * \note "category" format marker will be set to the category name for all of these messages
 * (see [AbstractStringAppender::setFormat](@ref AbstractStringAppender::setFormat)).
 *
 * \sa defaultCategory()
 * \sa registerCategoryAppender()
 * \sa logToGlobalInstance()
 */
void Logger::setDefaultCategory(const QString& category)
{
  Q_D(Logger);

  QMutexLocker locker(&d->loggerMutex);

  d->defaultCategory = category;
}

//! Returns default logging category name
/**
 * \sa setDefaultCategory
 */
QString Logger::defaultCategory() const
{
  Q_D(const Logger);
  return d->defaultCategory;
}

//! Links some logging category with the global logger instance appenders.
/**
 * If set to true, all log messages to the specified category appenders will also be written to the global logger instance appenders,
 * registered using registerAppender().
 *
 * By default, all messages to the specific category are written only to the specific category appenders
 * (registered using registerCategoryAppender()).
 *
 * \param category Category name
 * \param logToGlobal Link or onlink the category from global logger instance appender
 *
 * \sa globalInstance
 * \sa registerAppender
 * \sa registerCategoryAppender
 */
void Logger::logToGlobalInstance(const QString& category, bool logToGlobal)
{
  Q_D(Logger);

  if (this == globalInstance())
  {
    QMutexLocker locker(&d->loggerMutex);
    d->categories.insert(category, logToGlobal);
  }
  else
  {
    globalInstance()->logToGlobalInstance(category, logToGlobal);
  }
}


void Logger::write(const QDateTime& timeStamp, LogLevel logLevel, const char* file, int line, const char* function, const char* category,
                   const QString& message, bool fromLocalInstance)
{
  Q_D(Logger);

  QMutexLocker locker(&d->loggerMutex);

  QString logCategory = QString::fromLatin1(category);
  if (logCategory.isNull() && !d->defaultCategory.isNull())
    logCategory = d->defaultCategory;

  bool wasWritten = false;
  bool isGlobalInstance = this == globalInstance();
  bool linkedToGlobal = isGlobalInstance && d->categories.value(logCategory, false);

  if (!logCategory.isNull())
  {
    QList<AbstractAppender*> appenders = d->categoryAppenders.values(logCategory);
    if (appenders.length() == 0)
    {
      if (logCategory != d->defaultCategory && !linkedToGlobal && !fromLocalInstance && !d->noAppendersCategories.contains(logCategory))
      {
        std::cerr << "No appenders associated with category " << qPrintable(logCategory) << std::endl;
        d->noAppendersCategories.append(logCategory);
      }
    }
    else
    {
      foreach (AbstractAppender* appender, appenders)
        appender->write(timeStamp, logLevel, file, line, function, logCategory, message);
      wasWritten = true;
    }
  }

  // the default category is linked to the main logger appenders
  // global logger instance also writes all linked categories to the main appenders
  if (logCategory.isNull() || logCategory == d->defaultCategory || linkedToGlobal)
  {
    if (!d->appenders.isEmpty())
    {
      foreach (AbstractAppender* appender, d->appenders)
        appender->write(timeStamp, logLevel, file, line, function, logCategory, message);
      wasWritten = true;
    }
    else
    {
      static bool noAppendersWarningShown = false;
      if (!noAppendersWarningShown)
      {
#if defined(Q_OS_ANDROID)
        __android_log_write(ANDROID_LOG_WARN, "Logger", "No appenders registered with logger");
#else
        std::cerr << "No appenders registered with logger" << std::endl;
#endif
        noAppendersWarningShown = true;
      }
    }
  }

  // local logger instances send category messages to the global instance
  if (!isGlobalInstance)
  {
    if (!logCategory.isNull())
    {
      globalInstance()->write(timeStamp, logLevel, file, line, function, logCategory.toLatin1(), message, true);
      wasWritten = true;
    }

    if (d->writeDefaultCategoryToGlobalInstance && logCategory == d->defaultCategory)
    {
      globalInstance()->write(timeStamp, logLevel, file, line, function, nullptr, message, true);
      wasWritten = true;
    }
  }

  if (!wasWritten && !fromLocalInstance)
  {
    // Fallback
#if defined(Q_OS_ANDROID)
    QString result = QString(QLatin1String("<%2> %3")).arg(AbstractStringAppender::stripFunctionName(function)).arg(message);
    __android_log_write(AndroidAppender::androidLogPriority(logLevel), "Logger", qPrintable(result));
#else
    QString result = QString(QLatin1String("[%1] <%2> %3")).arg(levelToString(logLevel), -7)
                     .arg(AbstractStringAppender::stripFunctionName(function)).arg(message);
    std::cerr << qPrintable(result) << std::endl;
#endif
  }

  if (logLevel == Logger::Fatal)
    abort();
}


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
 * \param category - logging category (0 for default category)
 * \param message - log message
 *
 * \note Recording of the log record using the Logger::Fatal log level will lead to calling the STL abort()
 *       function, which will interrupt the running of your software and begin the writing of the core dump.
 *
 * \sa LogLevel
 * \sa LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL
 * \sa AbstractAppender
 */
void Logger::write(const QDateTime& timeStamp, LogLevel logLevel, const char* file, int line, const char* function, const char* category,
                   const QString& message)
{
  write(timeStamp, logLevel, file, line, function, category, message, /* fromLocalInstance = */ false);
}

/**
 * This is the overloaded function provided for the convinience. It behaves similar to the above function.
 *
 * This function uses the current timestamp obtained with \c QDateTime::currentDateTime().
 *
 * \sa write()
 */
void Logger::write(LogLevel logLevel, const char* file, int line, const char* function, const char* category,
                   const QString& message)
{
  write(QDateTime::currentDateTime(), logLevel, file, line, function, category, message);
}


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
void Logger::writeAssert(const char* file, int line, const char* function, const char* condition)
{
  write(Logger::Fatal, file, line, function, nullptr, QString("ASSERT: \"%1\"").arg(condition));
}


Logger* cuteLoggerInstance()
{
  return Logger::globalInstance();
}



void LoggerTimingHelper::start(const char* msg, ...)
{
  va_list va;
  va_start(va, msg);
#if QT_VERSION >= 0x050500
  m_block = QString().vasprintf(msg, va);
#else
  m_block = QString().vsprintf(msg, va);
#endif
  va_end(va);

  m_time.start();
}


void LoggerTimingHelper::start(const QString& block)
{
  m_block = block;
  m_time.start();
}


void LoggerTimingHelper::start(Logger::TimingMode mode, const QString& block)
{
  m_timingMode = mode;
  m_block = block;
  m_time.start();
}


LoggerTimingHelper::~LoggerTimingHelper()
{
  QString message;
  if (m_block.isEmpty())
    message = QString(QLatin1String("Function %1 finished in ")).arg(AbstractStringAppender::stripFunctionName(m_function));
  else
    message = QString(QLatin1String("\"%1\" finished in ")).arg(m_block);

  qint64 elapsed = m_time.elapsed();
  if (elapsed >= 10000 && m_timingMode == Logger::TimingAuto)
    message += QString(QLatin1String("%1 s.")).arg(elapsed / 1000);
  else
    message += QString(QLatin1String("%1 ms.")).arg(elapsed);

  m_logger->write(m_logLevel, m_file, m_line, m_function, nullptr, message);
}


CuteMessageLogger::~CuteMessageLogger()
{
  m_l->write(m_level, m_file, m_line, m_function, m_category, m_message);
}

void CuteMessageLogger::write(const char* msg, ...)
{
  va_list va;
  va_start(va, msg);
  m_message = QString::vasprintf(msg, va);
  va_end(va);
}


void CuteMessageLogger::write(const QString& msg)
{
  m_message = msg;
}


QDebug CuteMessageLogger::write()
{
  QDebug d(&m_message);
  return d;
}
