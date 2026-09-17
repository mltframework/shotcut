/*
 * Copyright (c) 2026 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "namedtemporaryfile.h"

#include <QFile>

#ifdef Q_OS_UNIX
#include <stdlib.h>
#include <unistd.h>
#endif

NamedTemporaryFile::~NamedTemporaryFile()
{
    if (!m_namedPath.isEmpty()) {
        close();
        QFile::remove(m_namedPath);
    }
}

QString NamedTemporaryFile::fileName() const
{
    return m_namedPath.isEmpty() ? QTemporaryFile::fileName() : m_namedPath;
}

bool NamedTemporaryFile::open(OpenMode flags)
{
    if (!QTemporaryFile::open(flags))
        return false;
    if (!QTemporaryFile::fileName().isEmpty())
        return true;
    QFile::close();
    return openWithMkstemp(flags);
}

bool NamedTemporaryFile::openWithMkstemp(OpenMode flags)
{
#ifdef Q_OS_UNIX
    QByteArray name = QFile::encodeName(fileTemplate());
    const int xPos = name.lastIndexOf("XXXXXX");
    if (xPos < 0)
        return false;
    const int fd = mkstemps(name.data(), int(name.size() - (xPos + 6)));
    if (fd < 0)
        return false;
    m_namedPath = QFile::decodeName(name);
    setAutoRemove(false);
    setFileName(m_namedPath);
    if (!QFile::open(fd, flags, QFileDevice::AutoCloseHandle)) {
        ::close(fd);
        QFile::remove(m_namedPath);
        m_namedPath.clear();
        return false;
    }
    return true;
#else
    Q_UNUSED(flags)
    return false;
#endif
}
