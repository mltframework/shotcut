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

#include "mcp/forumsearch.h"
#include "mcp/mcpclient.h"
#include "mcp/mcpserver.h"
#include "mcp/shotcuttools.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QUrlQuery>
#include <QtTest/QtTest>

namespace {
QJsonObject g_projectSnapshot;
}

namespace Shotcut {
namespace Mcp {
QJsonObject inspectProject()
{
    return g_projectSnapshot;
}
} // namespace Mcp
} // namespace Shotcut

class TestMcpServer : public QObject
{
    Q_OBJECT

private:
    Shotcut::Mcp::ToolRegistry m_registry;

private slots:
    void init()
    {
        Shotcut::Mcp::ToolDefinition pingTool;
        pingTool.name = QStringLiteral("ping");
        pingTool.description = QStringLiteral("Return a basic acknowledgement.");
        pingTool.parameters.insert("type", QStringLiteral("object"));
        pingTool.parameters.insert("properties", QJsonObject());
        pingTool.parameters.insert("required", QJsonArray());
        pingTool.handler = [](const QJsonObject &arguments) {
            QJsonObject response;
            response.insert("ok", true);
            response.insert("received", arguments);
            return response;
        };
        m_registry.registerTool(pingTool);

        Shotcut::Mcp::ToolDefinition echoTool;
        echoTool.name = QStringLiteral("echo");
        echoTool.description = QStringLiteral("Echo back the provided text.");
        QJsonObject properties;
        QJsonObject textSchema;
        textSchema.insert("type", QStringLiteral("string"));
        properties.insert("text", textSchema);
        echoTool.parameters.insert("type", QStringLiteral("object"));
        echoTool.parameters.insert("properties", properties);
        echoTool.parameters.insert("required", QJsonArray());
        echoTool.handler = [](const QJsonObject &arguments) {
            QJsonObject response;
            response.insert("echo", arguments.value("text").toString());
            return response;
        };
        m_registry.registerTool(echoTool);
    }

    void toolsListReturnsRegisteredTools()
    {
        const QList<Shotcut::Mcp::ToolDefinition> tools = m_registry.tools();
        QVERIFY(tools.size() >= 2);

        bool foundPing = false;
        bool foundEcho = false;
        for (const Shotcut::Mcp::ToolDefinition &tool : tools) {
            if (tool.name == QStringLiteral("ping"))
                foundPing = true;
            if (tool.name == QStringLiteral("echo"))
                foundEcho = true;
        }

        QVERIFY(foundPing);
        QVERIFY(foundEcho);
    }

    void toolCallReturnsStructuredContent()
    {
        QJsonObject arguments;
        arguments.insert("text", QStringLiteral("hello from test"));

        const QJsonObject result = m_registry.call(QStringLiteral("echo"), arguments);
        QVERIFY(result.contains("echo"));
        QCOMPARE(result.value("echo").toString(), QStringLiteral("hello from test"));
    }

    void searchToolsHaveDistinctDescriptions()
    {
        const auto documentation = Shotcut::Mcp::makeDocumentationSearchTool();
        const auto discussion = Shotcut::Mcp::makeForumSearchTool();
        QCOMPARE(documentation.name, QStringLiteral("shotcut_documentation_search"));
        QCOMPARE(discussion.name, QStringLiteral("shotcut_forum_search"));
        QVERIFY(documentation.description.contains(QStringLiteral("English documentation")));
        QVERIFY(discussion.description.contains(QStringLiteral("excluding English documentation")));
        QCOMPARE(documentation.parameters, discussion.parameters);
        const auto tools = Shotcut::Mcp::createShotcutToolRegistry().tools();
        QStringList names;
        for (const auto &tool : tools)
            names.append(tool.name);
        QVERIFY(names.contains(documentation.name));
        QVERIFY(names.contains(discussion.name));
    }

    void forumSearchScopesQuery_data()
    {
        QTest::addColumn<bool>("documentation");
        QTest::addColumn<QString>("expectedQuery");
        QTest::newRow("documentation") << true << QStringLiteral("audio #docs:docs-english");
        QTest::newRow("discussion") << false << QStringLiteral("audio -#docs:docs-english");
    }

    void forumSearchScopesQuery()
    {
        QFETCH(bool, documentation);
        QFETCH(QString, expectedQuery);
        Shotcut::Mcp::ForumSearch search;
        auto *network = search.findChild<QNetworkAccessManager *>();
        QVERIFY(network);
        // Keep this request local and cancel before processing network events.
        network->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QStringLiteral("127.0.0.1"), 9));
        search.search(QStringLiteral("audio"),
                      documentation ? Shotcut::Mcp::ForumSearch::Scope::Documentation
                                    : Shotcut::Mcp::ForumSearch::Scope::Discussion);
        const auto replies = network->findChildren<QNetworkReply *>();
        QCOMPARE(replies.size(), 1);
        const auto url = replies.first()->request().url();
        search.cancel();
        QCOMPARE(url.host(), QStringLiteral("forum.shotcut.org"));
        QCOMPARE(url.path(), QStringLiteral("/search.json"));
        QCOMPARE(QUrlQuery(url).queryItemValue(QStringLiteral("q")), expectedQuery);
    }

    void searchToolsRejectEmptyQueries_data()
    {
        QTest::addColumn<QString>("toolName");
        QTest::newRow("documentation") << QStringLiteral("shotcut_documentation_search");
        QTest::newRow("discussion") << QStringLiteral("shotcut_forum_search");
    }

    void searchToolsRejectEmptyQueries()
    {
        QFETCH(QString, toolName);
        Shotcut::Mcp::McpServer server(Shotcut::Mcp::createShotcutToolRegistry());
        QVERIFY(server.listen());
        Shotcut::Mcp::McpClient client(server.boundPort());
        QSignalSpy responseSpy(&client, &Shotcut::Mcp::McpClient::requestCompleted);
        const auto requestId = client.callTool(toolName, QJsonObject());
        QTRY_COMPARE_WITH_TIMEOUT(responseSpy.count(), 1, 3000);
        const auto arguments = responseSpy.takeFirst();
        QCOMPARE(arguments.at(0).toULongLong(), requestId);
        QVERIFY(arguments.at(2).toString().isEmpty());
        const auto result = qvariant_cast<QJsonObject>(arguments.at(1)).value("result").toObject();
        QVERIFY(result.value("isError").toBool());
        QCOMPARE(result.value("structuredContent").toObject().value("error").toString(),
                 QStringLiteral("The query argument is required."));
    }

    void clientUsesServerForToolListAndCall()
    {
        Shotcut::Mcp::McpServer server(m_registry);
        QVERIFY(server.listen());
        QVERIFY(server.boundPort() > 0);

        Shotcut::Mcp::McpClient client(server.boundPort());
        QSignalSpy responseSpy(&client, &Shotcut::Mcp::McpClient::requestCompleted);
        QVERIFY(responseSpy.isValid());

        const quint64 initializeRequestId = client.initialize();
        QTRY_VERIFY_WITH_TIMEOUT(responseSpy.count() >= 1, 3000);
        const QList<QVariant> initializeArguments = responseSpy.takeFirst();
        QCOMPARE(initializeArguments.at(0).toULongLong(), initializeRequestId);
        QVERIFY(initializeArguments.at(2).toString().isEmpty());
        QCOMPARE(qvariant_cast<QJsonObject>(initializeArguments.at(1))
                     .value("result").toObject().value("protocolVersion").toString(),
                 QStringLiteral("2025-11-25"));

        const quint64 listRequestId = client.toolsList();
        QTRY_VERIFY_WITH_TIMEOUT(responseSpy.count() >= 1, 3000);
        const QList<QVariant> listArguments = responseSpy.takeFirst();
        QCOMPARE(listArguments.at(0).toULongLong(), listRequestId);
        const QJsonObject listResponse = qvariant_cast<QJsonObject>(listArguments.at(1));
        QVERIFY(listArguments.at(2).toString().isEmpty());
        const QJsonArray listedTools = listResponse.value("result").toObject().value("tools").toArray();
        QVERIFY(listedTools.size() >= 2);

        QJsonObject arguments;
        arguments.insert("text", QStringLiteral("round trip"));
        const quint64 callRequestId = client.callTool(QStringLiteral("echo"), arguments);
        QTRY_VERIFY_WITH_TIMEOUT(responseSpy.count() >= 1, 3000);
        const QList<QVariant> callArguments = responseSpy.takeFirst();
        QCOMPARE(callArguments.at(0).toULongLong(), callRequestId);
        const QJsonObject callResponse = qvariant_cast<QJsonObject>(callArguments.at(1));
        QVERIFY(callArguments.at(2).toString().isEmpty());
        const QJsonObject structuredContent
            = callResponse.value("result").toObject().value("structuredContent").toObject();
        QCOMPARE(structuredContent.value("echo").toString(), QStringLiteral("round trip"));
    }

    void httpTransportRejectsForeignOriginAndUnsupportedGet()
    {
        Shotcut::Mcp::McpServer server(m_registry);
        QVERIFY(server.listen());

        QNetworkAccessManager network;
        const QUrl endpoint(QStringLiteral("http://127.0.0.1:%1/mcp").arg(server.boundPort()));
        auto waitForStatus = [](QNetworkReply *reply) {
            QEventLoop eventLoop;
            QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
            if (reply->isRunning())
                eventLoop.exec();
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            reply->deleteLater();
            return status;
        };

        QNetworkRequest foreignOriginRequest(endpoint);
        foreignOriginRequest.setRawHeader("Origin", "http://example.com");
        foreignOriginRequest.setRawHeader("Accept", "application/json, text/event-stream");
        foreignOriginRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                                       QStringLiteral("application/json"));
        const QByteArray initializeMessage
            = R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"test","version":"1"}}})";
        QCOMPARE(waitForStatus(network.post(foreignOriginRequest, initializeMessage)), 403);

        QNetworkRequest getRequest(endpoint);
        getRequest.setRawHeader("Origin", "http://localhost");
        getRequest.setRawHeader("Accept", "text/event-stream");
        QCOMPARE(waitForStatus(network.get(getRequest)), 405);
    }

    void clientsStayWithTheirOwnServerInstance()
    {
        Shotcut::Mcp::ToolRegistry secondRegistry = m_registry;
        Shotcut::Mcp::ToolDefinition secondEcho = secondRegistry.tool(QStringLiteral("echo"));
        secondEcho.handler = [](const QJsonObject &) {
            return QJsonObject{{"echo", QStringLiteral("second instance")}};
        };
        secondRegistry.registerTool(secondEcho);

        Shotcut::Mcp::McpServer firstServer(m_registry);
        Shotcut::Mcp::McpServer secondServer(secondRegistry);
        QVERIFY(firstServer.listen());
        QVERIFY(secondServer.listen());
        QVERIFY(firstServer.boundPort() > 0);
        QVERIFY(secondServer.boundPort() > 0);
        QVERIFY(firstServer.boundPort() != secondServer.boundPort());

        Shotcut::Mcp::McpClient firstClient(firstServer.boundPort());
        Shotcut::Mcp::McpClient secondClient(secondServer.boundPort());
        QSignalSpy firstSpy(&firstClient, &Shotcut::Mcp::McpClient::requestCompleted);
        QSignalSpy secondSpy(&secondClient, &Shotcut::Mcp::McpClient::requestCompleted);
        QVERIFY(firstSpy.isValid());
        QVERIFY(secondSpy.isValid());

        firstClient.initialize();
        secondClient.initialize();
        QTRY_COMPARE_WITH_TIMEOUT(firstSpy.count(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(secondSpy.count(), 1, 3000);
        firstSpy.clear();
        secondSpy.clear();

        firstClient.callTool(QStringLiteral("echo"), QJsonObject());
        secondClient.callTool(QStringLiteral("echo"), QJsonObject());
        QTRY_COMPARE_WITH_TIMEOUT(firstSpy.count(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(secondSpy.count(), 1, 3000);

        const QJsonObject firstResult
            = qvariant_cast<QJsonObject>(firstSpy.takeFirst().at(1))
                  .value("result").toObject().value("structuredContent").toObject();
        const QJsonObject secondResult
            = qvariant_cast<QJsonObject>(secondSpy.takeFirst().at(1))
                  .value("result").toObject().value("structuredContent").toObject();
        QCOMPARE(firstResult.value("echo").toString(), QStringLiteral(""));
        QCOMPARE(secondResult.value("echo").toString(), QStringLiteral("second instance"));
    }

    void timelineSummaryOmitsTracksByDefault()
    {
        const QJsonArray tracks{QJsonObject{{"name", QStringLiteral("V1")}},
                                QJsonObject{{"name", QStringLiteral("A1")}}};
        g_projectSnapshot = QJsonObject{
            {"active_context", QStringLiteral("timeline")},
            {"has_timeline", true},
            {"has_playlist", false},
            {"timeline", QJsonObject{{"track_count", 2}, {"tracks", tracks}}},
        };

        const QJsonObject result = Shotcut::Mcp::shotcutTimelineSummaryTool(QJsonObject());
        const QJsonObject timeline = result.value("timeline").toObject();
        QVERIFY(!result.value("include_tracks").toBool());
        QVERIFY(!timeline.contains("tracks"));
        QCOMPARE(timeline.value("track_count").toInt(), 2);
    }

    void timelineSummaryIncludesTracksWhenRequested()
    {
        const QJsonArray tracks{QJsonObject{{"name", QStringLiteral("V1")}},
                                QJsonObject{{"name", QStringLiteral("A1")}}};
        g_projectSnapshot = QJsonObject{
            {"active_context", QStringLiteral("timeline")},
            {"has_timeline", true},
            {"has_playlist", false},
            {"timeline", QJsonObject{{"track_count", 2}, {"tracks", tracks}}},
        };
        const QJsonObject arguments{{"include_tracks", true}};

        const QJsonObject result = Shotcut::Mcp::shotcutTimelineSummaryTool(arguments);
        const QJsonObject timeline = result.value("timeline").toObject();
        QVERIFY(result.value("include_tracks").toBool());
        QCOMPARE(timeline.value("tracks").toArray().size(), 2);
        QCOMPARE(timeline.value("track_count").toInt(), 2);
    }
};

QTEST_GUILESS_MAIN(TestMcpServer)

#include "test_mcpserver.moc"
