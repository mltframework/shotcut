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

#ifndef COPRODUCERFORUMSEARCH_H
#define COPRODUCERFORUMSEARCH_H

#include <QList>
#include <QObject>
#include <QString>

class QNetworkReply;

namespace Coproducer {

// One retrieved source, kept structured (rather than pre-formatted text) so
// the caller can number sources itself and later match model citations back
// to a trusted URL instead of rendering whatever URL the model outputs.
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
    explicit ForumSearch(QObject *parent = nullptr);
    void search(const QString &query);

signals:
    void resultsReady(const QList<Coproducer::ForumSource> &sources, const QString &error);

private slots:
    void onFinished(QNetworkReply *reply);

private:
    QString cleanForumText(const QString &html) const;
    void finishIfReady();

    int m_pending = 0;
    QString m_query;
    QList<ForumSource> m_results;
    QStringList m_errors;
};

} // namespace Coproducer

#endif // COPRODUCERFORUMSEARCH_H
