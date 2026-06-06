#ifndef ANDROIDAPPENDER_H
#define ANDROIDAPPENDER_H

// Local
#include <AbstractStringAppender.h>


/**
 * @brief A logging appender for Android logcat.
 *
 * This class implements a logging appender that sends log messages to the
 * Android logcat system. It inherits from AbstractStringAppender and provides
 * Android-specific logging functionality by converting log levels and formatting
 * messages appropriately for the Android environment.
 *
 * The appender maps CuteLogger's log levels to corresponding Android log priorities
 * and handles the formatting and output of log messages to the Android system log.
 *
 * @see AbstractStringAppender for the base class interface
 * @see Logger::LogLevel for available log levels
 *
 * @note This appender is only effective on Android platforms.
 */
class AndroidAppender : public AbstractStringAppender
{
  public:
    AndroidAppender();

    static int androidLogPriority(Logger::LogLevel);

  protected:
    void append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                const char* function, const QString& category, const QString& message);

};

#endif // ANDROIDAPPENDER_H
