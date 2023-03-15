/*
  Copyright (c) 2010 Boris Moiseev (cyberbobs at gmail dot com) Nikolay Matyunin (matyunin.n at gmail dot com)

  Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).

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
#include "AbstractStringAppender.h"

// Qt
#include <QReadLocker>
#include <QWriteLocker>
#include <QDateTime>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QThread>


/**
 * \class AbstractStringAppender
 *
 * \brief The AbstractStringAppender class provides a convinient base for appenders working with plain text formatted
 *        logs.
 *
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


const char formattingMarker = '%';


//! Constructs a new string appender object
AbstractStringAppender::AbstractStringAppender()
  : m_format(QLatin1String("%{time}{yyyy-MM-ddTHH:mm:ss.zzz} [%{type:-7}] <%{function}> %{message}\n"))
{}


//! Returns the current log format string.
/**
 * The default format is set to "%{time}{yyyy-MM-ddTHH:mm:ss.zzz} [%{type:-7}] <%{function}> %{message}\n". You can set a different log record
 * format using the setFormat() function.
 *
 * \sa setFormat(const QString&)
 */
QString AbstractStringAppender::format() const
{
  QReadLocker locker(&m_formatLock);
  return m_format;
}


//! Sets the logging format for writing strings to the log target with this appender.
/**
 * The string format seems to be very common to those developers who have used a standart sprintf function.
 *
 * Log output format is a simple QString with the special markers (starting with % sign) which will be replaced with
 * it's internal meaning when writing a log record.
 *
 * Controlling marker begins with the percent sign (%) which is followed by the command inside {} brackets
 * (the command describes, what will be put to log record instead of marker).
 * Optional field width argument may be specified right after the command (through the colon symbol before the closing bracket)
 * Some commands requires an additional formatting argument (in the second {} brackets).
 *
 * Field width argument works almost identically to the \c QString::arg() \c fieldWidth argument (and uses it
 * internally). For example, \c "%{type:-7}" will be replaced with the left padded debug level of the message
 * (\c "Debug  ") or something. For the more detailed description of it you may consider to look to the Qt
 * Reference Documentation.
 *
 * Supported marker commands are:
 *   \arg \c %{time} - timestamp. You may specify your custom timestamp format using the second {} brackets after the marker,
 *           timestamp format here will be similiar to those used in QDateTime::toString() function. For example,
 *           "%{time}{dd-MM-yyyy, HH:mm}" may be replaced with "17-12-2010, 20:17" depending on current date and time.
 *           The default format used here is "HH:mm:ss.zzz".
 *   \arg \c %{type} - Log level. Possible log levels are shown in the Logger::LogLevel enumerator.
 *   \arg \c %{Type} - Uppercased log level.
 *   \arg \c %{typeOne} - One letter log level.
 *   \arg \c %{TypeOne} - One uppercase letter log level.
 *   \arg \c %{File} - Full source file name (with path) of the file that requested log recording. Uses the \c __FILE__
 *           preprocessor macro.
 *   \arg \c %{file} - Short file name (with stripped path).
 *   \arg \c %{line} - Line number in the source file. Uses the \c __LINE__ preprocessor macro.
 *   \arg \c %{Function} - Name of function that called on of the LOG_* macros. Uses the \c Q_FUNC_INFO macro provided with
 *           Qt.
 *   \arg \c %{function} - Similiar to the %{Function}, but the function name is stripped using stripFunctionName
 *   \arg \c %{message} - The log message sent by the caller.
 *   \arg \c %{category} - The log category.
 *   \arg \c %{appname} - Application name (returned by QCoreApplication::applicationName() function).
 *   \arg \c %{pid} - Application pid (returned by QCoreApplication::applicationPid() function).
 *   \arg \c %{threadid} - ID of current thread.
 *   \arg \c %% - Convinient marker that is replaced with the single \c % mark.
 *
 * \note Format doesn't add \c '\\n' to the end of the format line. Please consider adding it manually.
 *
 * \sa format()
 * \sa stripFunctionName()
 * \sa Logger::LogLevel
 */
void AbstractStringAppender::setFormat(const QString& format)
{
  QWriteLocker locker(&m_formatLock);
  m_format = format;
}


//! Strips the long function signature (as added by Q_FUNC_INFO macro)
/**
 * The string processing drops the returning type, arguments and template parameters of function. It is definitely
 * useful for enchancing the log output readability.
 * \return stripped function name
 */
QString AbstractStringAppender::stripFunctionName(const char* name)
{
  return QString::fromLatin1(qCleanupFuncinfo(name));
}


// The function was backported from Qt5 sources (qlogging.h)
QByteArray AbstractStringAppender::qCleanupFuncinfo(const char* name)
{
  QByteArray info(name);

  // Strip the function info down to the base function name
  // note that this throws away the template definitions,
  // the parameter types (overloads) and any const/volatile qualifiers.
  if (info.isEmpty())
      return info;

  int pos;

  // skip trailing [with XXX] for templates (gcc)
  pos = info.size() - 1;
  if (info.endsWith(']')) {
      while (--pos) {
          if (info.at(pos) == '[')
              info.truncate(pos);
      }
  }

  bool hasLambda = false;
  QRegularExpression lambdaRegex("::<lambda\\(.*?\\)>");
  QRegularExpressionMatch match = lambdaRegex.match(QString::fromLatin1(info));
  int lambdaIndex = match.capturedStart();
  if (lambdaIndex != -1)
  {
    hasLambda = true;
    info.remove(lambdaIndex, match.capturedLength());
  }

  // operator names with '(', ')', '<', '>' in it
  static const char operator_call[] = "operator()";
  static const char operator_lessThan[] = "operator<";
  static const char operator_greaterThan[] = "operator>";
  static const char operator_lessThanEqual[] = "operator<=";
  static const char operator_greaterThanEqual[] = "operator>=";

  // canonize operator names
  info.replace("operator ", "operator");

  // remove argument list
  forever {
      int parencount = 0;
      pos = info.lastIndexOf(')');
      if (pos == -1) {
          // Don't know how to parse this function name
          return info;
      }

      // find the beginning of the argument list
      --pos;
      ++parencount;
      while (pos && parencount) {
          if (info.at(pos) == ')')
              ++parencount;
          else if (info.at(pos) == '(')
              --parencount;
          --pos;
      }
      if (parencount != 0)
          return info;

      info.truncate(++pos);

      if (info.at(pos - 1) == ')') {
          if (info.indexOf(operator_call) == pos - (int)strlen(operator_call))
              break;

          // this function returns a pointer to a function
          // and we matched the arguments of the return type's parameter list
          // try again
          info.remove(0, info.indexOf('('));
          info.chop(1);
          continue;
      } else {
          break;
      }
  }

  if (hasLambda)
    info.append("::lambda");

  // find the beginning of the function name
  int parencount = 0;
  int templatecount = 0;
  --pos;

  // make sure special characters in operator names are kept
  if (pos > -1) {
      switch (info.at(pos)) {
      case ')':
          if (info.indexOf(operator_call) == pos - (int)strlen(operator_call) + 1)
              pos -= 2;
          break;
      case '<':
          if (info.indexOf(operator_lessThan) == pos - (int)strlen(operator_lessThan) + 1)
              --pos;
          break;
      case '>':
          if (info.indexOf(operator_greaterThan) == pos - (int)strlen(operator_greaterThan) + 1)
              --pos;
          break;
      case '=': {
          int operatorLength = (int)strlen(operator_lessThanEqual);
          if (info.indexOf(operator_lessThanEqual) == pos - operatorLength + 1)
              pos -= 2;
          else if (info.indexOf(operator_greaterThanEqual) == pos - operatorLength + 1)
              pos -= 2;
          break;
      }
      default:
          break;
      }
  }

  while (pos > -1) {
      if (parencount < 0 || templatecount < 0)
          return info;

      char c = info.at(pos);
      if (c == ')')
          ++parencount;
      else if (c == '(')
          --parencount;
      else if (c == '>')
          ++templatecount;
      else if (c == '<')
          --templatecount;
      else if (c == ' ' && templatecount == 0 && parencount == 0)
          break;

      --pos;
  }
  info = info.mid(pos + 1);

  // remove trailing '*', '&' that are part of the return argument
  while ((info.at(0) == '*')
         || (info.at(0) == '&'))
      info = info.mid(1);

  // we have the full function name now.
  // clean up the templates
  while ((pos = info.lastIndexOf('>')) != -1) {
      if (!info.contains('<'))
          break;

      // find the matching close
      int end = pos;
      templatecount = 1;
      --pos;
      while (pos && templatecount) {
          char c = info.at(pos);
          if (c == '>')
              ++templatecount;
          else if (c == '<')
              --templatecount;
          --pos;
      }
      ++pos;
      info.remove(pos, end - pos + 1);
  }

  return info;
}


//! Returns the string to record to the logging target, formatted according to the format().
/**
 * \sa format()
 * \sa setFormat(const QString&)
 */
QString AbstractStringAppender::formattedString(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file,
                                                int line, const char* function, const QString& category, const QString& message) const
{
  QString f = format();
  const int size = f.size();

  QString result;

  int i = 0;
  while (i < f.size())
  {
    QChar c = f.at(i);

    // We will silently ignore the broken % marker at the end of string
    if (c != QLatin1Char(formattingMarker) || (i + 2) >= size)
    {
      result.append(c);
    }
    else
    {
      i += 2;
      QChar currentChar = f.at(i);
      QString command;
      int fieldWidth = 0;

      if (currentChar.isLetter())
      {
        command.append(currentChar);
        int j = 1;
        while ((i + j) < size && f.at(i + j).isLetter())
        {
          command.append(f.at(i+j));
          j++;
        }

        i+=j;
        currentChar = f.at(i);

        // Check for the padding instruction
        if (currentChar == QLatin1Char(':'))
        {
          currentChar = f.at(++i);
          if (currentChar.isDigit() || currentChar.category() == QChar::Punctuation_Dash)
          {
            int j = 1;
            while ((i + j) < size && f.at(i + j).isDigit())
              j++;
            fieldWidth = f.mid(i, j).toInt();

            i += j;
          }
        }
      }

      // Log record chunk to insert instead of formatting instruction
      QString chunk;

      // Time stamp
      if (command == QLatin1String("time"))
      {
        if (f.at(i + 1) == QLatin1Char('{'))
        {
          int j = 1;
          while ((i + 2 + j) < size && f.at(i + 2 + j) != QLatin1Char('}'))
            j++;

          if ((i + 2 + j) < size)
          {
            chunk = timeStamp.toString(f.mid(i + 2, j));

            i += j;
            i += 2;
          }
        }

        if (chunk.isNull())
          chunk = timeStamp.toString(QLatin1String("HH:mm:ss.zzz"));
      }

      // Log level
      else if (command == QLatin1String("type"))
        chunk = Logger::levelToString(logLevel);

      // Uppercased log level
      else if (command == QLatin1String("Type"))
        chunk = Logger::levelToString(logLevel).toUpper();

      // One letter log level
      else if (command == QLatin1String("typeOne"))
          chunk = Logger::levelToString(logLevel).left(1).toLower();

      // One uppercase letter log level
      else if (command == QLatin1String("TypeOne"))
          chunk = Logger::levelToString(logLevel).left(1).toUpper();

      // Filename
      else if (command == QLatin1String("File"))
        chunk = QLatin1String(file);

      // Filename without a path
      else if (command == QLatin1String("file"))
        chunk = QString(QLatin1String(file)).section(QRegularExpression("[/\\\\]"), -1);

      // Source line number
      else if (command == QLatin1String("line"))
        chunk = QString::number(line);

      // Function name, as returned by Q_FUNC_INFO
      else if (command == QLatin1String("Function"))
        chunk = QString::fromLatin1(function);

      // Stripped function name
      else if (command == QLatin1String("function"))
        chunk = stripFunctionName(function);

      // Log message
      else if (command == QLatin1String("message"))
        chunk = message;

      else if (command == QLatin1String("category"))
        chunk = category;

      // Application pid
      else if (command == QLatin1String("pid"))
        chunk = QString::number(QCoreApplication::applicationPid());

      // Appplication name
      else if (command == QLatin1String("appname"))
        chunk = QCoreApplication::applicationName();

      // Thread ID (duplicates Qt5 threadid debbuging way)
      else if (command == QLatin1String("threadid"))
        chunk = QLatin1String("0x") + QString::number(qlonglong(QThread::currentThread()->currentThread()), 16);

      // We simply replace the double formatting marker (%) with one
      else if (command == QString(formattingMarker))
        chunk = QLatin1Char(formattingMarker);

      // Do not process any unknown commands
      else
      {
        chunk = QString(formattingMarker);
        chunk.append(command);
      }

      result.append(QString(QLatin1String("%1")).arg(chunk, fieldWidth));
    }

    ++i;
  }

  return result;
}
