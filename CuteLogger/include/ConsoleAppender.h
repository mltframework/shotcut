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
#ifndef CONSOLEAPPENDER_H
#define CONSOLEAPPENDER_H

#include "CuteLogger_global.h"
#include <AbstractStringAppender.h>


class CUTELOGGERSHARED_EXPORT ConsoleAppender : public AbstractStringAppender
{
  public:
    ConsoleAppender();
    virtual QString format() const;
    void ignoreEnvironmentPattern(bool ignore);

  protected:
    virtual void append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
                        const char* function, const QString& category, const QString& message);

  private:
    bool m_ignoreEnvPattern;
};

#endif // CONSOLEAPPENDER_H
