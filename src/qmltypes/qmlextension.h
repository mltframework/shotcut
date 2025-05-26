/*
 * Copyright (c) 2025 Meltytech, LLC
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

#ifndef QMLEXTENSION_H
#define QMLEXTENSION_H

#include <QObject>
#include <QQmlListProperty>
#include <QString>

class QmlExtensionFile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER m_name NOTIFY changed)
    Q_PROPERTY(QString description MEMBER m_description NOTIFY changed)
    Q_PROPERTY(QString file MEMBER m_file NOTIFY changed)
    Q_PROPERTY(QString url MEMBER m_url NOTIFY changed)
    Q_PROPERTY(QString size MEMBER m_size NOTIFY changed)

public:
    explicit QmlExtensionFile(QObject *parent = 0);

    QString name() const { return m_name; }
    QString description() const { return m_description; }
    QString file() const { return m_file; }
    QString url() const { return m_url; }
    QString size() const { return m_size; }

signals:
    void changed();

private:
    QString m_name;
    QString m_description;
    QString m_file;
    QString m_url;
    QString m_size;
};

class QmlExtension : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY changed)
    Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY changed)
    Q_PROPERTY(QQmlListProperty<QmlExtensionFile> files READ files NOTIFY changed)

public:
    static QmlExtension *load(const QString &id);

    explicit QmlExtension(QObject *parent = 0);

    QString name() const { return m_name; }
    void setName(const QString &);
    QString version() const { return m_version; }
    void setVersion(const QString &);
    QQmlListProperty<QmlExtensionFile> files()
    {
        return QQmlListProperty<QmlExtensionFile>(this, &m_files);
    }
    int fileCount() const { return m_files.count(); }
    QmlExtensionFile *file(int index) const { return m_files[index]; }
    QString localPath(int index);
    bool downloaded(int index);

signals:
    void changed();

private:
    QString m_name;
    QString m_version;
    QList<QmlExtensionFile *> m_files;
};

#endif // QMLEXTENSION_H
