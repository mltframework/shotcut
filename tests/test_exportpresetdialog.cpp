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

#include "addencodepresetdialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class TestExportPresetDialog : public QObject
{
    Q_OBJECT

private slots:
    void savesNames_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("fileName");
        QTest::newRow("plain") << QStringLiteral("H264") << QStringLiteral("H264");
        QTest::newRow("slash") << QStringLiteral("H264/1080p") << QStringLiteral("H264%2F1080p");
        QTest::newRow("backslash")
            << QStringLiteral("H264\\1080p") << QStringLiteral("H264%5C1080p");
        QTest::newRow("percent") << QStringLiteral("H264%2F1080p")
                                 << QStringLiteral("H264%252F1080p");
        QTest::newRow("space") << QStringLiteral("H264 1080p") << QStringLiteral("H264%201080p");
        QTest::newRow("category") << QStringLiteral("Web)H264/1080p")
                                  << QStringLiteral("Web%29H264%2F1080p");
        QTest::newRow("unicode") << QString::fromUtf8("Видео/1080p")
                                 << QStringLiteral("%D0%92%D0%B8%D0%B4%D0%B5%D0%BE%2F1080p");
    }

    void savesNames()
    {
        QFETCH(QString, name);
        QFETCH(QString, fileName);
        AddEncodePresetDialog dialog;
        const QString properties("vcodec=libx264\nacodec=aac");
        dialog.setProperties(properties);
        auto nameEdit = dialog.findChild<QLineEdit *>("nameEdit");
        QVERIFY(nameEdit);
        nameEdit->setText(name);
        dialog.findChild<QDialogButtonBox *>("buttonBox")->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(dialog.result(), int(QDialog::Accepted));
        QCOMPARE(dialog.presetName(), fileName);

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QDir dir(directory.path());
        QFile file(dir.filePath(dialog.presetName()));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QCOMPARE(file.write(dialog.properties().toUtf8()), properties.toUtf8().size());
        file.close();
        const auto entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        QCOMPARE(entries, QStringList(fileName));
        QFile reloaded(dir.filePath(entries.first()));
        QVERIFY(reloaded.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(reloaded.readAll(), properties.toUtf8());
        reloaded.close();
        QVERIFY(dir.remove(entries.first()));
        QVERIFY(dir.entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty());
    }
};

QTEST_MAIN(TestExportPresetDialog)
#include "test_exportpresetdialog.moc"
