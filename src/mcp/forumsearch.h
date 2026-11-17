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

#ifndef SHOTCUT_MCP_FORUMSEARCH_H
#define SHOTCUT_MCP_FORUMSEARCH_H

#include <QList>
#include <QObject>
#include <QString>

class QNetworkReply;
class QTimer;

namespace Shotcut {
namespace Mcp {

struct ForumSource
{
    QString title;
    QString url;
    QString excerpt;
};

class ForumSearch : public QObject
{
    Q_OBJECT

public:
    enum class Scope { Documentation, Discussion };

    explicit ForumSearch(QObject *parent = nullptr);
    void search(const QString &query, Scope scope, int timeoutMs = 30000);
    void cancel();

signals:
    void resultsReady(const QList<Shotcut::Mcp::ForumSource> &sources, const QString &error);

private slots:
    void onFinished(QNetworkReply *reply);
    void onTimeout();

private:
    QString cleanForumText(const QString &html) const;
    void finishIfReady();

    int m_pending = 0;
    quint64 m_searchGeneration = 0;
    bool m_finished = true;
    QTimer *m_timeoutTimer = nullptr;
    QList<ForumSource> m_results;
    QStringList m_errors;
};

} // namespace Mcp
} // namespace Shotcut

#endif // SHOTCUT_MCP_FORUMSEARCH_H