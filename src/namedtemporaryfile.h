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

#ifndef NAMEDTEMPORARYFILE_H
#define NAMEDTEMPORARYFILE_H

#include <QTemporaryFile>

// QTemporaryFile that always has a fileName() after a successful open().
// If Qt leaves the file unnamed (Linux O_TMPFILE on some filesystems),
// open() creates a named file with mkstemps() instead.
class NamedTemporaryFile : public QTemporaryFile
{
public:
    using QTemporaryFile::open;
    using QTemporaryFile::QTemporaryFile;
    ~NamedTemporaryFile() override;

    QString fileName() const override;

protected:
    bool open(OpenMode flags) override;

private:
    bool openWithMkstemp(OpenMode flags);

    QString m_namedPath;
};

#endif // NAMEDTEMPORARYFILE_H
