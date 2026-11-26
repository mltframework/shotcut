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

#include "api/filteranimation.h"

#include <MltFactory.h>
#include <QtTest>

#include <limits>

using namespace Shotcut::Api;

class TestFilterAnimation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { QVERIFY(Mlt::Factory::init()); }
    void cleanupTestCase() { Mlt::Factory::close(); }

    void numericRoundTrip()
    {
        Mlt::Properties properties;
        QString error;
        const QVariantList
            keys{QVariantMap{{"frame", 99}, {"value", 2.0}, {"interpolation", "smoothNatural"}},
                 QVariantMap{{"frame", 0}, {"value", 0.5}, {"interpolation", "discrete"}},
                 QVariantMap{{"frame", 50}, {"value", 1.0}, {"interpolation", "linear"}}};
        QVERIFY2(FilterAnimation::write(properties, "level", false, 100, keys, error),
                 qPrintable(error));
        const auto result = FilterAnimation::read(properties, "level", false, 100);
        QCOMPARE(result.size(), 3);
        QCOMPARE(result[0].toMap().value("frame").toInt(), 0);
        QCOMPARE(result[0].toMap().value("value").toDouble(), 0.5);
        QCOMPARE(result[1].toMap().value("frame").toInt(), 50);
        QCOMPARE(result[2].toMap().value("interpolation").toString(), QString("smoothNatural"));
        QCOMPARE(properties.anim_get_double("level", 25, 100), 0.5);
    }

    void rectangleRoundTrip()
    {
        Mlt::Properties properties;
        QString error;
        const QVariantMap rect{{"x", 10.5},
                               {"y", -5.0},
                               {"width", 640.0},
                               {"height", 360.0},
                               {"opacity", 0.5}};
        const QVariantList keys{QVariantMap{{"frame", 0}, {"value", rect}},
                                QVariantMap{{"frame", 24},
                                            {"value", rect},
                                            {"interpolation", "smooth"}}};
        QVERIFY2(FilterAnimation::write(properties, "rect", true, 25, keys, error),
                 qPrintable(error));
        const auto result = FilterAnimation::read(properties, "rect", true, 25);
        QCOMPARE(result.size(), 2);
        QCOMPARE(result[0].toMap().value("value").toMap(), rect);
        QCOMPARE(result[1].toMap().value("interpolation").toString(), QString("smooth"));
    }

    void staticValuesHaveNoKeys()
    {
        Mlt::Properties properties;
        properties.set("level", 1.0);
        QVERIFY(FilterAnimation::read(properties, "level", false, 25).isEmpty());
        QVERIFY(FilterAnimation::read(properties, "missing", false, 25).isEmpty());
    }

    void allInterpolationsRoundTrip()
    {
        const QStringList names{"discrete", "linear", "smooth", "smoothNatural", "smoothTight"};
        QStringList types = names;
        const QStringList easing{"sinusoidal",
                                 "quadratic",
                                 "cubic",
                                 "quartic",
                                 "quintic",
                                 "exponential",
                                 "circular",
                                 "back",
                                 "elastic",
                                 "bounce"};
        for (const auto &name : easing)
            for (const auto &suffix : {QString("In"), QString("Out"), QString("InOut")})
                types.append(name + suffix);
        for (const auto &type : types) {
            Mlt::Properties properties;
            QString error;
            const QVariantList keys{QVariantMap{{"frame", 0},
                                                {"value", 0.0},
                                                {"interpolation", type}},
                                    QVariantMap{{"frame", 24}, {"value", 1.0}}};
            QVERIFY2(FilterAnimation::write(properties, "level", false, 25, keys, error),
                     qPrintable(error));
            const auto result = FilterAnimation::read(properties, "level", false, 25);
            QCOMPARE(result[0].toMap().value("interpolation").toString(), type);
        }
    }

    void rejectsMalformedRectangles()
    {
        const QVariantMap valid{{"x", 0}, {"y", 0}, {"width", 100}, {"height", 100}, {"opacity", 1}};
        for (const auto &field : valid.keys()) {
            for (const QVariant &bad :
                 {QVariant(), QVariant("1"), QVariant(std::numeric_limits<double>::quiet_NaN())}) {
                auto rect = valid;
                rect[field] = bad;
                Mlt::Properties properties;
                properties.set("rect", "1 2 3 4 1");
                QString error;
                QVERIFY(!FilterAnimation::write(properties,
                                                "rect",
                                                true,
                                                25,
                                                {QVariantMap{{"frame", 0}, {"value", rect}}},
                                                error));
                QVERIFY(!error.isEmpty());
                QCOMPARE(QString::fromUtf8(properties.get("rect")), QString("1 2 3 4 1"));
            }
        }
    }

    void keyCountLimit()
    {
        QVariantList keys;
        for (int i = 0; i < 10000; ++i)
            keys.append(QVariantMap{{"frame", i}, {"value", double(i)}});
        Mlt::Properties properties;
        QString error;
        QVERIFY2(FilterAnimation::write(properties, "level", false, 10001, keys, error),
                 qPrintable(error));
        QCOMPARE(FilterAnimation::read(properties, "level", false, 10001).size(), 10000);
        const QString before = QString::fromUtf8(properties.get("level"));
        keys.append(QVariantMap{{"frame", 10000}, {"value", 1.0}});
        QVERIFY(!FilterAnimation::write(properties, "level", false, 10001, keys, error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(QString::fromUtf8(properties.get("level")), before);
    }

    void rejectsInvalidKeys_data()
    {
        QTest::addColumn<QVariantList>("keys");
        QTest::newRow("empty") << QVariantList{};
        QTest::newRow("negative") << QVariantList{QVariantMap{{"frame", -1}, {"value", 1.0}}};
        QTest::newRow("end") << QVariantList{QVariantMap{{"frame", 25}, {"value", 1.0}}};
        QTest::newRow("fraction") << QVariantList{QVariantMap{{"frame", 0.5}, {"value", 1.0}}};
        QTest::newRow("boolean") << QVariantList{QVariantMap{{"frame", 0}, {"value", true}}};
        QTest::newRow("string") << QVariantList{QVariantMap{{"frame", 0}, {"value", "1"}}};
        QTest::newRow("infinity") << QVariantList{
            QVariantMap{{"frame", 0}, {"value", std::numeric_limits<double>::infinity()}}};
        QTest::newRow("unknown-type")
            << QVariantList{QVariantMap{{"frame", 0}, {"value", 1.0}, {"interpolation", "bogus"}}};
        QTest::newRow("unknown-field")
            << QVariantList{QVariantMap{{"frame", 0}, {"value", 1.0}, {"typo", "linear"}}};
        QTest::newRow("duplicate") << QVariantList{QVariantMap{{"frame", 0}, {"value", 1.0}},
                                                   QVariantMap{{"frame", 0}, {"value", 2.0}}};
    }

    void rejectsInvalidKeys()
    {
        QFETCH(QVariantList, keys);
        Mlt::Properties properties;
        properties.set("level", 0.75);
        QString error;
        QVERIFY(!FilterAnimation::write(properties, "level", false, 25, keys, error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(properties.get_double("level"), 0.75);
    }
};

QTEST_APPLESS_MAIN(TestFilterAnimation)
#include "test_filteranimation.moc"
