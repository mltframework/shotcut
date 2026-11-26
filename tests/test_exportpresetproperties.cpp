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

#include "exportpreset.h"

#include <MltFactory.h>
#include <QDomDocument>
#include <QFile>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>

class TestExportPresetProperties : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { QVERIFY(Mlt::Factory::init(nullptr)); }

    void cleanupTestCase() { Mlt::Factory::close(); }

    void detectsImageSequences_data()
    {
        QTest::addColumn<QString>("codec");
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("expected");
        for (const auto *codec : {"bmp", "dpx", "png", "ppm", "targa", "tiff"})
            QTest::newRow(codec) << QString::fromLatin1(codec) << QString() << true;
        QTest::newRow("image2") << QString() << QStringLiteral("image2") << true;
        QTest::newRow("webp-sequence")
            << QStringLiteral("libwebp") << QStringLiteral("image2") << true;
        QTest::newRow("jpeg-sequence")
            << QStringLiteral("mjpeg") << QStringLiteral("image2") << true;
        QTest::newRow("webp") << QStringLiteral("libwebp") << QStringLiteral("webp") << false;
        QTest::newRow("mjpeg-video") << QStringLiteral("mjpeg") << QStringLiteral("avi") << false;
        QTest::newRow("h264") << QStringLiteral("libx264") << QStringLiteral("mp4") << false;
        QTest::newRow("empty") << QString() << QString() << false;
    }

    void detectsImageSequences()
    {
        QFETCH(QString, codec);
        QFETCH(QString, format);
        QFETCH(bool, expected);
        Mlt::Properties properties;
        properties.set("vcodec", codec.toLatin1().constData());
        properties.set("f", format.toLatin1().constData());
        QCOMPARE(ExportPreset::isImageSequence(properties), expected);
        properties.set("vn", 1);
        QVERIFY(!ExportPreset::isImageSequence(properties));
        properties.set("vn", 0);
        properties.set("video_off", 1);
        QVERIFY(!ExportPreset::isImageSequence(properties));
    }

    void listsStableStockPresetIds()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto presets = ExportPreset::list(directory.path());
        QSet<QString> ids;
        int stockCount = 0;
        for (const auto &entry : presets) {
            const QString id = entry.toMap().value("id").toString();
            QVERIFY2(!ids.contains(id),
                     qPrintable(QStringLiteral("Duplicate preset ID: %1").arg(id)));
            ids.insert(id);
            if (id.startsWith("stock:")) {
                QVERIFY2(id.startsWith("stock:consumer/avformat/"), qPrintable(id));
                ++stockCount;
            }
        }
        QVERIFY2(stockCount > 0,
                 "No stock presets found; check the installed MLT data or MLT_DATA.");
        QVERIFY(ids.contains("default"));
        QVERIFY(ids.contains("stock:consumer/avformat/x264-medium"));
        QVERIFY(ids.contains("stock:consumer/avformat/atsc_1080p_25/DNxHD"));
    }

    void resolvesStockPresets_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<QString>("codec");
        QTest::addColumn<QString>("extension");
        QTest::newRow("h264") << QStringLiteral("stock:consumer/avformat/x264-medium")
                              << QStringLiteral("libx264") << QStringLiteral("mp4");
        QTest::newRow("dnxhd") << QStringLiteral("stock:consumer/avformat/atsc_1080p_25/DNxHD")
                               << QStringLiteral("dnxhd") << QStringLiteral("mov");
    }

    void resolvesStockPresets()
    {
        QFETCH(QString, id);
        QFETCH(QString, codec);
        QFETCH(QString, extension);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ExportPreset::ResolvedPreset preset;
        QString error;
        QVERIFY2(ExportPreset::resolve(id, preset, error, directory.path(), 2), qPrintable(error));
        QCOMPARE(QString::fromLatin1(preset.properties.get("vcodec")), codec);
        QCOMPARE(QString::fromLatin1(preset.properties.get("f")), extension);
        QCOMPARE(preset.extension, extension);
        QVERIFY(!preset.twoPass);
        QVERIFY(!preset.imageSequence);
        QVERIFY(!preset.properties.get("meta.preset.extension"));
        QVERIFY(!preset.properties.get("meta.preset.name"));
        if (codec == "dnxhd") {
            QCOMPARE(preset.properties.get_int("width"), 1920);
            QCOMPARE(preset.properties.get_int("height"), 1080);
            QCOMPARE(preset.properties.get_int("frame_rate_num"), 25);
            QCOMPARE(preset.properties.get_int("frame_rate_den"), 1);
            QCOMPARE(preset.properties.get_int("progressive"), 1);
            QVERIFY(qAbs(preset.properties.get_double("aspect") - 16.0 / 9.0) < 0.000001);
        }
    }

    void rejectsShortenedStockPresetId()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ExportPreset::ResolvedPreset preset;
        QString error;
        QVERIFY(!ExportPreset::resolve("stock:x264-medium", preset, error, directory.path(), 2));
        QVERIFY(error.contains("stock:x264-medium"));
    }

    void resolvesDefaultAndCustomPresets()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile file(directory.filePath("Two Pass"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write("f=mp4\nvcodec=libx264\nvb=1M\nv2pass=1\n") > 0);
        file.close();

        ExportPreset::ResolvedPreset preset;
        QString error;
        QVERIFY2(ExportPreset::resolve("default", preset, error, directory.path(), 4),
                 qPrintable(error));
        QCOMPARE(preset.properties.get_int("channels"), 4);
        QCOMPARE(QString::fromLatin1(preset.properties.get("channel_layout")),
                 QStringLiteral("quad"));
        QVERIFY2(ExportPreset::resolve("custom:Two Pass", preset, error, directory.path(), 2),
                 qPrintable(error));
        QVERIFY(preset.twoPass);
        QCOMPARE(QString::fromLatin1(preset.properties.get("vb")), QStringLiteral("1M"));
        QVERIFY(!preset.properties.get("crf"));
        QVERIFY(!ExportPreset::resolve("custom:../Two Pass", preset, error, directory.path(), 2));
    }

    void preservesConsumerFieldsAndStripsHints()
    {
        Mlt::Properties stored;
        stored.set("f", "matroska");
        stored.set("vcodec", "libx264");
        stored.set("preset", "veryslow");
        stored.set("vb", "1400k");
        stored.set("meta.preset.name", "Friendly name");
        stored.set("meta.preset.extension", "mkv");
        stored.set("_shotcut:internal", "hidden");
        stored.set("v2pass", "true");

        Mlt::Properties consumer;
        bool twoPass = false;
        QString error;
        QVERIFY2(ExportPreset::resolve(stored, consumer, twoPass, error), qPrintable(error));
        QVERIFY(twoPass);
        QCOMPARE(QString::fromLatin1(consumer.get("f")), QStringLiteral("matroska"));
        QCOMPARE(QString::fromLatin1(consumer.get("vcodec")), QStringLiteral("libx264"));
        QCOMPARE(QString::fromLatin1(consumer.get("preset")), QStringLiteral("veryslow"));
        QCOMPARE(QString::fromLatin1(consumer.get("vb")), QStringLiteral("1400k"));
        QVERIFY(!consumer.get("crf"));
        QVERIFY(!consumer.get("meta.preset.name"));
        QVERIFY(!consumer.get("meta.preset.extension"));
        QVERIFY(!consumer.get("_shotcut:internal"));
        QVERIFY(!consumer.get("v2pass"));
    }

    void interpretsPassMarker()
    {
        Mlt::Properties stored;
        stored.set("vcodec", "libx264");
        stored.set("pass", 2);
        Mlt::Properties consumer;
        bool twoPass = false;
        QString error;

        QVERIFY2(ExportPreset::resolve(stored, consumer, twoPass, error), qPrintable(error));
        QVERIFY(twoPass);
        QVERIFY(!consumer.get("pass"));

        stored.clear("pass");
        stored.set("pass", 0);
        QVERIFY2(ExportPreset::resolve(stored, consumer, twoPass, error), qPrintable(error));
        QVERIFY(!twoPass);
        QVERIFY(!consumer.get("pass"));
    }

    void rejectsInvalidOrConflictingPassRules()
    {
        Mlt::Properties stored;
        Mlt::Properties consumer;
        bool twoPass = false;
        QString error;

        stored.set("v2pass", "maybe");
        QVERIFY(!ExportPreset::resolve(stored, consumer, twoPass, error));
        QVERIFY(error.contains("v2pass"));

        stored.clear("v2pass");
        error.clear();
        stored.set("pass", 3);
        QVERIFY(!ExportPreset::resolve(stored, consumer, twoPass, error));
        QVERIFY(error.contains("pass"));

        stored.clear("pass");
        error.clear();
        stored.set("v2pass", 0);
        stored.set("pass", 1);
        QVERIFY(!ExportPreset::resolve(stored, consumer, twoPass, error));
        QVERIFY(error.contains("conflict"));
    }

    void preservesExplicitEncoderWithoutSubstitution()
    {
        Mlt::Properties stored;
        stored.set("vcodec", "h264_nvenc");
        Mlt::Properties consumer;
        bool twoPass = false;
        QString error;

        QVERIFY2(ExportPreset::resolve(stored, consumer, twoPass, error), qPrintable(error));
        QVERIFY(!twoPass);
        QCOMPARE(QString::fromLatin1(consumer.get("vcodec")), QStringLiteral("h264_nvenc"));

        stored.set("v2pass", 1);
        QVERIFY(!ExportPreset::resolve(stored, consumer, twoPass, error));
        QVERIFY(error.contains("hardware encoder"));
    }

    void buildsTwoPassConsumerProperties()
    {
        QDomDocument document;
        auto consumer = document.createElement("consumer");
        consumer.setAttribute("acodec", "aac");
        ExportPreset::configurePass(consumer, "C:/exports/output.mp4", "libx264", 1, true);
        QCOMPARE(consumer.attribute("pass"), QStringLiteral("1"));
        QCOMPARE(consumer.attribute("passlogfile"),
                 QStringLiteral("C:/exports/output.mp4_2pass.log"));
        QCOMPARE(consumer.attribute("fastfirstpass"), QStringLiteral("1"));
        QCOMPARE(consumer.attribute("an"), QStringLiteral("1"));
        QVERIFY(!consumer.hasAttribute("acodec"));

        consumer.setAttribute("acodec", "aac");
        ExportPreset::configurePass(consumer, "C:/exports/output.mp4", "libx264", 2, true);
        QCOMPARE(consumer.attribute("pass"), QStringLiteral("2"));
        QCOMPARE(consumer.attribute("acodec"), QStringLiteral("aac"));
        QVERIFY(!consumer.hasAttribute("fastfirstpass"));

        consumer.setAttribute("fastfirstpass", "7");
        ExportPreset::configurePass(consumer, "C:/exports/output.mp4", "libx264", 0, true);
        QCOMPARE(consumer.attribute("fastfirstpass"), QStringLiteral("7"));
        ExportPreset::configurePass(consumer, "C:/exports/output.mp4", "libx264", 0, false);
        QVERIFY(!consumer.hasAttribute("fastfirstpass"));
    }

    void preservesCodecSpecificPassOptions()
    {
        QDomDocument document;
        auto x265 = document.createElement("consumer");
        x265.setAttribute("x265-params", "crf=24");
        ExportPreset::configurePass(x265, "C:/exports/output.mp4", "libx265", 1, true);
        QVERIFY(x265.attribute("x265-params").startsWith("pass=1:stats=C\\:/"));
        QVERIFY(x265.attribute("x265-params").endsWith(":crf=24"));

        auto svt = document.createElement("consumer");
        svt.setAttribute("svtav1-params", "tune=0");
        ExportPreset::configurePass(svt, "C:/exports/output.mp4", "libsvtav1", 2, true);
        const auto params = svt.attribute("svtav1-params");
        QVERIFY(params.startsWith("passes=2:pass=2:stats=C\\:/"));
        QVERIFY(params.endsWith(":tune=0"));
    }

    void preservesGuiCodecSpecificFastFirstPass()
    {
        QDomDocument document;
        for (const auto &codec : {QStringLiteral("libx265"), QStringLiteral("libsvtav1")}) {
            for (int pass = 0; pass <= 2; ++pass) {
                auto consumer = document.createElement("consumer");
                consumer.setAttribute("fastfirstpass", "7");
                ExportPreset::configurePass(consumer, "C:/exports/output.mp4", codec, pass, false);
                QCOMPARE(consumer.attribute("fastfirstpass"), QStringLiteral("7"));
            }
        }
    }
};

QTEST_APPLESS_MAIN(TestExportPresetProperties)
#include "test_exportpresetproperties.moc"
