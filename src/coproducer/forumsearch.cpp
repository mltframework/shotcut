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

#include "forumsearch.h"

#include "Logger.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrlQuery>

namespace Coproducer {

ForumSearch::ForumSearch(QObject *parent)
    : QObject(parent)
{
    auto network = new QNetworkAccessManager(this);
    connect(network, &QNetworkAccessManager::finished, this, &ForumSearch::onFinished);
}

void ForumSearch::search(const QString &query)
{
    m_results.clear();
    m_errors.clear();
    m_pending = 0;
    m_query = query;

    auto network = findChild<QNetworkAccessManager *>();
    if (!network) {
        emit resultsReady({}, QString());
        return;
    }

    QUrl url(QStringLiteral("https://forum.shotcut.org/search.json"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("q"), query);
    url.setQuery(params);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Shotcut Coproducer/1.0");
    auto reply = network->get(request);
    reply->setProperty("searchType", QStringLiteral("forum"));
    ++m_pending;
}

void ForumSearch::onFinished(QNetworkReply *reply)
{
    const auto body = reply->readAll();
    const auto type = reply->property("searchType").toString();
    if (reply->error() == QNetworkReply::NoError) {
        if (type == QStringLiteral("forum")) {
            const auto topics
                = QJsonDocument::fromJson(body).object().value(QStringLiteral("topics")).toArray();
            int fetched = 0;
            auto network = findChild<QNetworkAccessManager *>();
            for (const auto &value : topics) {
                const auto topic = value.toObject();
                const auto title = topic.value(QStringLiteral("title")).toString();
                const auto slug = topic.value(QStringLiteral("slug")).toString();
                const auto id = topic.value(QStringLiteral("id")).toInt();
                if (title.isEmpty() || slug.isEmpty() || id <= 0 || fetched == 4)
                    continue;

                const auto url
                    = QStringLiteral("https://forum.shotcut.org/t/%1/%2.json").arg(slug).arg(id);
                auto topicReply = network->get(QNetworkRequest(QUrl(url)));
                topicReply->setProperty("searchType", QStringLiteral("forumTopic"));
                topicReply->setProperty("topicTitle", title);
                topicReply->setProperty("topicUrl",
                                        QStringLiteral("https://forum.shotcut.org/t/%1/%2")
                                            .arg(slug)
                                            .arg(id));
                ++m_pending;
                ++fetched;
            }
            if (fetched == 0)
                m_errors.append(QStringLiteral("Forum search returned no topics to fetch."));
        } else if (type == QStringLiteral("forumTopic")) {
            const auto root = QJsonDocument::fromJson(body).object();
            const auto posts = root.value(QStringLiteral("post_stream"))
                                   .toObject()
                                   .value(QStringLiteral("posts"))
                                   .toArray();
            const auto post = posts.isEmpty() ? QJsonObject() : posts.first().toObject();
            const auto excerpt = cleanForumText(post.value(QStringLiteral("cooked")).toString());
            const auto title = reply->property("topicTitle").toString();
            const auto url = reply->property("topicUrl").toString();
            if (!excerpt.isEmpty()) {
                m_results.append({title, url, excerpt});
            } else {
                m_errors.append(
                    QStringLiteral("Forum topic returned no readable excerpt: %1").arg(url));
            }
        }
    } else {
        m_errors.append(QStringLiteral("%1 search failed: %2").arg(type, reply->errorString()));
        LOG_WARNING() << "Coproducer" << type << "search failed:" << reply->errorString();
    }
    reply->deleteLater();
    --m_pending;
    finishIfReady();
}

void ForumSearch::finishIfReady()
{
    if (m_pending > 0)
        return;
    LOG_DEBUG() << "Coproducer: search complete; source count=" << m_results.size()
                << "errors=" << m_errors.size();
    emit resultsReady(m_results, m_errors.join(QStringLiteral("\n")));
}

QString ForumSearch::cleanForumText(const QString &html) const
{
    auto text = html;
    text.replace(QRegularExpression(QStringLiteral("<[^>]*>")), QStringLiteral(" "));
    text.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    text.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    text.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    text.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    text.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return text.trimmed().left(1200);
}

} // namespace Coproducer
