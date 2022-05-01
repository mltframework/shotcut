/*
 * Copyright (c) 2018-2021 Meltytech, LLC
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

#ifndef POSTJOBACTION_H
#define POSTJOBACTION_H

#include <QString>
#include <QUuid>

class PostJobAction
{
public:
    virtual ~PostJobAction() {}
    virtual void doAction() = 0;
};

class FilePropertiesPostJobAction : public PostJobAction
{
public:
    FilePropertiesPostJobAction(const QString &srcFile, const QString &dstFile)
        : m_srcFile(srcFile)
        , m_dstFile(dstFile)
    {}
    virtual ~FilePropertiesPostJobAction() {}
    virtual void doAction();

protected:
    QString m_srcFile;
    QString m_dstFile;
};

class OpenPostJobAction : public FilePropertiesPostJobAction
{
public:
    OpenPostJobAction(const QString &srcFile, const QString &dstFile, const QString &fileNameToRemove)
        : FilePropertiesPostJobAction(srcFile, dstFile)
        , m_fileNameToRemove(fileNameToRemove)
    {}
    void doAction();

private:
    QString m_fileNameToRemove;
};

class ReplaceOnePostJobAction : public FilePropertiesPostJobAction
{
public:
    ReplaceOnePostJobAction(const QString &srcFile, const QString &dstFile,
                            const QString &fileNameToRemove, const QUuid &srcUuid, int in)
        : FilePropertiesPostJobAction(srcFile, dstFile)
        , m_fileNameToRemove(fileNameToRemove)
        , m_uuid(srcUuid)
        , m_in(in)
    {}
    void doAction();

private:
    QString m_fileNameToRemove;
    QUuid m_uuid;
    int m_in;
};

class ReplaceAllPostJobAction : public FilePropertiesPostJobAction
{
public:
    ReplaceAllPostJobAction(const QString &srcFile, const QString &dstFile, const QString &srcHash)
        : FilePropertiesPostJobAction(srcFile, dstFile)
        , m_hash(srcHash)
    {}
    void doAction();

private:
    QString m_hash;
};

class ProxyReplacePostJobAction : public FilePropertiesPostJobAction
{
public:
    ProxyReplacePostJobAction(const QString &srcFile, const QString &dstFile, const QString &srcHash)
        : FilePropertiesPostJobAction(srcFile, dstFile)
        , m_srcFile(srcFile)
        , m_dstFile(dstFile)
        , m_hash(srcHash)
    {}
    void doAction();

private:
    QString m_srcFile;
    QString m_dstFile;
    QString m_hash;
};

class ProxyFinalizePostJobAction : public FilePropertiesPostJobAction
{
public:
    ProxyFinalizePostJobAction(const QString &srcFile, const QString &dstFile)
        : FilePropertiesPostJobAction(srcFile, dstFile)
        , m_dstFile(dstFile)
    {}
    void doAction();

private:
    QString m_dstFile;
};

#endif // POSTJOBACTION_H
