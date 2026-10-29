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

#include "subtitles.h"

#include <QtTest>

namespace {

std::string toCrlf(const std::string &text)
{
    std::string out;
    out.reserve(text.size() * 2);
    for (char c : text) {
        if (c == '\n') {
            out += "\r\n";
        } else {
            out += c;
        }
    }
    return out;
}

} // namespace

class TestSubtitles : public QObject
{
    Q_OBJECT

private slots:
    void extraBlankSeparatorsKeepLaterCues()
    {
        const std::string srt = "1\n"
                                "00:00:00,000 --> 00:00:01,000\n"
                                "Hello\n"
                                "\n"
                                "\n"
                                "2\n"
                                "00:00:01,000 --> 00:00:02,000\n"
                                "World\n"
                                "\n"
                                "3\n"
                                "00:00:02,500 --> 00:00:03,500\n"
                                "Later\n";
        const auto items = Subtitles::readFromSrtString(srt);
        QCOMPARE(items.size(), std::size_t{3});
        QCOMPARE(items[0].start, int64_t{0});
        QCOMPARE(items[0].end, int64_t{1000});
        QCOMPARE(items[0].text, std::string("Hello"));
        QCOMPARE(items[1].start, int64_t{1000});
        QCOMPARE(items[1].end, int64_t{2000});
        QCOMPARE(items[1].text, std::string("World"));
        QCOMPARE(items[2].start, int64_t{2500});
        QCOMPARE(items[2].end, int64_t{3500});
        QCOMPARE(items[2].text, std::string("Later"));
    }

    void crlfExtraBlankSeparatorsKeepLaterCues()
    {
        const std::string srt = "1\n"
                                "00:00:00,000 --> 00:00:01,000\n"
                                "Hello\n"
                                "\n"
                                "\n"
                                "2\n"
                                "00:00:01,000 --> 00:00:02,000\n"
                                "World\n"
                                "\n"
                                "3\n"
                                "00:00:02,500 --> 00:00:03,500\n"
                                "Later\n";
        const auto items = Subtitles::readFromSrtString(toCrlf(srt));
        QCOMPARE(items.size(), std::size_t{3});
        QCOMPARE(items[0].start, int64_t{0});
        QCOMPARE(items[0].end, int64_t{1000});
        QCOMPARE(items[0].text, std::string("Hello"));
        QCOMPARE(items[1].start, int64_t{1000});
        QCOMPARE(items[1].end, int64_t{2000});
        QCOMPARE(items[1].text, std::string("World"));
        QCOMPARE(items[2].start, int64_t{2500});
        QCOMPARE(items[2].end, int64_t{3500});
        QCOMPARE(items[2].text, std::string("Later"));
    }

    void blankLineEndsCueAndKeepsNext()
    {
        const std::string srt = "1\n"
                                "00:00:00,000 --> 00:00:01,500\n"
                                "Line one\n"
                                "\n"
                                "Line two\n"
                                "\n"
                                "2\n"
                                "00:00:02,000 --> 00:00:03,000\n"
                                "Next\n";
        const auto items = Subtitles::readFromSrtString(srt);
        QCOMPARE(items.size(), std::size_t{2});
        QCOMPARE(items[0].start, int64_t{0});
        QCOMPARE(items[0].end, int64_t{1500});
        QCOMPARE(items[0].text, std::string("Line one"));
        QVERIFY(items[0].text.find("Line two") == std::string::npos);
        QCOMPARE(items[1].start, int64_t{2000});
        QCOMPARE(items[1].end, int64_t{3000});
        QCOMPARE(items[1].text, std::string("Next"));
    }

    void writeTrimsTrailingNewlinesAndReturnsSrt()
    {
        Subtitles::SubtitleVector written;
        Subtitles::SubtitleItem item;
        item.start = 0;
        item.end = 1000;
        item.text = "Hello\n\n\r";
        written.push_back(item);
        item.start = 1500;
        item.end = 2500;
        item.text = "Tail";
        written.push_back(item);

        std::string srt = "unchanged";
        QVERIFY(Subtitles::writeToSrtString(srt, written));
        QVERIFY(srt != "unchanged");
        QVERIFY(srt.find("Hello") != std::string::npos);
        QVERIFY(srt.find("Tail") != std::string::npos);
        QVERIFY(srt.find("Hello\n\n2\n") != std::string::npos);
        QVERIFY(srt.find("Hello\n\n\n") == std::string::npos);

        const auto items = Subtitles::readFromSrtString(srt);
        QCOMPARE(items[0].start, int64_t{0});
        QCOMPARE(items[0].end, int64_t{1000});
        QCOMPARE(items[0].text, std::string("Hello"));
        QCOMPARE(items[1].start, int64_t{1500});
        QCOMPARE(items[1].end, int64_t{2500});
        QCOMPARE(items[1].text, std::string("Tail"));
    }
};

QTEST_APPLESS_MAIN(TestSubtitles)

#include "test_subtitles.moc"
