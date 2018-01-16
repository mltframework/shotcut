#ifndef ROLLINGFILEAPPENDER_H
#define ROLLINGFILEAPPENDER_H

#include <QDateTime>

#include <FileAppender.h>

/*!
 * \brief The RollingFileAppender class extends FileAppender so that the underlying file is rolled over at a user chosen frequency.
 *
 * The class is based on Log4Qt.DailyRollingFileAppender class (http://log4qt.sourceforge.net/)
 * and has the same date pattern format.
 *
 * For example, if the fileName is set to /foo/bar and the DatePattern set to the daily rollover ('.'yyyy-MM-dd'.log'), on 2014-02-16 at midnight,
 * the logging file /foo/bar.log will be copied to /foo/bar.2014-02-16.log and logging for 2014-02-17 will continue in /foo/bar
 * until it rolls over the next day.
 *
 * The logFilesLimit parameter is used to automatically delete the oldest log files in the directory during rollover
 * (so no more than logFilesLimit recent log files exist in the directory at any moment).
 * \sa setDatePattern(DatePattern), setLogFilesLimit(int)
 */
class CUTELOGGERSHARED_EXPORT RollingFileAppender : public FileAppender
{
  public:
    /*!
     * The enum DatePattern defines constants for date patterns.
     * \sa setDatePattern(DatePattern)
     */
    enum DatePattern
    {
      /*! The minutely date pattern string is "'.'yyyy-MM-dd-hh-mm". */
      MinutelyRollover = 0,
      /*! The hourly date pattern string is "'.'yyyy-MM-dd-hh". */
      HourlyRollover,
      /*! The half-daily date pattern string is "'.'yyyy-MM-dd-a". */
      HalfDailyRollover,
      /*! The daily date pattern string is "'.'yyyy-MM-dd". */
      DailyRollover,
      /*! The weekly date pattern string is "'.'yyyy-ww". */
      WeeklyRollover,
      /*! The monthly date pattern string is "'.'yyyy-MM". */
      MonthlyRollover
    };
    Q_ENUMS(DatePattern)

    RollingFileAppender(const QString& fileName = QString());

    DatePattern datePattern() const;
    void setDatePattern(DatePattern datePattern);
    void setDatePattern(const QString& datePattern);

    QString datePatternString() const;

    void setLogFilesLimit(int limit);
    int logFilesLimit() const;

  protected:
    virtual void append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                        const char* function, const QString& category, const QString& message);

  private:
    void rollOver();
    void computeRollOverTime();
    void computeFrequency();
    void removeOldFiles();
    void setDatePatternString(const QString& datePatternString);

    QString m_datePatternString;
    DatePattern m_frequency;

    QDateTime m_rollOverTime;
    QString m_rollOverSuffix;
    int m_logFilesLimit;
    mutable QMutex m_rollingMutex;
};

#endif // ROLLINGFILEAPPENDER_H
