// Local
#include "AndroidAppender.h"

// Android
#include <android/log.h>


AndroidAppender::AndroidAppender()
{
  setFormat(QLatin1String("<%{function}> %{message}\n"));
}


int AndroidAppender::androidLogPriority(Logger::LogLevel logLevel)
{
  switch (logLevel)
  {
    case Logger::Trace:
      return ANDROID_LOG_VERBOSE;
    case Logger::Debug:
      return ANDROID_LOG_DEBUG;
    case Logger::Info:
      return ANDROID_LOG_INFO;
    case Logger::Warning:
      return ANDROID_LOG_WARN;
    case Logger::Error:
      return ANDROID_LOG_ERROR;
    case Logger::Fatal:
      return ANDROID_LOG_FATAL;
  }

  // Just in case
  return ANDROID_LOG_DEFAULT;
}


void AndroidAppender::append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                             const char* function, const QString& category, const QString& message)
{
  QString msg = formattedString(timeStamp, logLevel, file, line, function, category, message);
  QString cat = category;
  if (cat.isEmpty())
    cat = QLatin1String("Logger");

  __android_log_write(androidLogPriority(logLevel), qPrintable(cat), qPrintable(msg));
}

