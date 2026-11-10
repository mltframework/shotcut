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

#include "api/transaction.h"
#include "dialogs/edittransactiondialog.h"
#include "mainwindow.h"

#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QTimer>
#include <QUndoStack>
#include <QtTest>

// Transaction tests must use their injected undo stack, never the application singleton.
MainWindow &MainWindow::singleton()
{
    qFatal("Unexpected MainWindow access in transaction test");
}

QUndoStack *MainWindow::undoStack() const
{
    qFatal("Unexpected MainWindow undo stack access in transaction test");
}

using Shotcut::Api::Transaction;

class ModalUndoCommand : public QUndoCommand
{
public:
    ModalUndoCommand(int &value, EditTransactionDialog &dialog, bool &blockedDuringUndo)
        : m_value(value)
        , m_dialog(dialog)
        , m_blockedDuringUndo(blockedDuringUndo)
    {}

    void redo() override { ++m_value; }
    void undo() override
    {
        QCoreApplication::processEvents();
        m_blockedDuringUndo = QApplication::activeModalWidget() == &m_dialog;
        --m_value;
    }

private:
    int &m_value;
    EditTransactionDialog &m_dialog;
    bool &m_blockedDuringUndo;
};

class TestEditTransactionDialog : public QObject
{
    Q_OBJECT

private slots:
    void blocksOtherWindows()
    {
        QPushButton main("Main window");
        QPushButton other("Other window");
        main.show();
        other.show();
        QSignalSpy mainClicks(&main, &QPushButton::clicked);
        QSignalSpy otherClicks(&other, &QPushButton::clicked);
        QCoreApplication::processEvents();
        QTest::mouseClick(main.windowHandle(), Qt::LeftButton);
        QTest::mouseClick(other.windowHandle(), Qt::LeftButton);
        QCOMPARE(mainClicks.count(), 1);
        QCOMPARE(otherClicks.count(), 1);
        EditTransactionDialog dialog(&main);
        dialog.begin("Editing");
        QCoreApplication::processEvents();
        QTest::mouseClick(main.windowHandle(), Qt::LeftButton);
        QTest::mouseClick(other.windowHandle(), Qt::LeftButton);
        QCOMPARE(mainClicks.count(), 1);
        QCOMPARE(otherClicks.count(), 1);
        dialog.hide();
        QCoreApplication::processEvents();
        QTest::mouseClick(main.windowHandle(), Qt::LeftButton);
        QTest::mouseClick(other.windowHandle(), Qt::LeftButton);
        QCOMPARE(mainClicks.count(), 2);
        QCOMPARE(otherClicks.count(), 2);
    }

    void transactionLifetime_data()
    {
        QTest::addColumn<bool>("rollback");
        QTest::addColumn<bool>("script");
        QTest::newRow("mcp-commit") << false << false;
        QTest::newRow("mcp-rollback") << true << false;
        QTest::newRow("script-commit") << false << true;
        QTest::newRow("script-rollback") << true << true;
    }

    void transactionLifetime()
    {
        QFETCH(bool, rollback);
        QFETCH(bool, script);
        QWidget window;
        window.show();
        EditTransactionDialog dialog(&window);
        auto &transaction = Transaction::singleton();
        QUndoStack stack;
        transaction.setUndoStack(&stack);
        connect(&transaction, &Transaction::opened, &dialog, &EditTransactionDialog::begin);
        connect(&transaction, &Transaction::closed, &dialog, &QWidget::hide);
        QSignalSpy opened(&transaction, &Transaction::opened);
        QSignalSpy closed(&transaction, &Transaction::closed);
        int value = 0;
        bool blockedDuringUndo = false;
        transaction.begin("Outer",
                          script ? Transaction::Origin::Script : Transaction::Origin::Coproducer);
        transaction.begin("Inner");
        transaction.touch();
        stack.push(new ModalUndoCommand(value, dialog, blockedDuringUndo));
        const auto nested = transaction.end();
        const bool stillBlocked = QApplication::activeModalWidget() == &dialog;
        if (rollback)
            transaction.fail("Test rollback");
        const auto result = transaction.end();
        transaction.setUndoStack(nullptr);
        QCOMPARE(nested, Transaction::Result::Nested);
        QVERIFY(stillBlocked);
        QCOMPARE(opened.count(), 1);
        QCOMPARE(closed.count(), 1);
        QCOMPARE(result,
                 rollback ? Transaction::Result::RolledBack : Transaction::Result::Committed);
        QCOMPARE(value, rollback ? 0 : 1);
        QCOMPARE(stack.count(), rollback ? 0 : 1);
        if (rollback)
            QVERIFY(blockedDuringUndo);
        QVERIFY(!dialog.isVisible());
        QVERIFY(!QApplication::activeModalWidget());
    }

    void autoTransactionWaitsForCall()
    {
        QWidget window;
        window.show();
        EditTransactionDialog dialog(&window);
        auto &transaction = Transaction::singleton();
        QUndoStack stack;
        transaction.setUndoStack(&stack);
        connect(&transaction, &Transaction::opened, &dialog, &EditTransactionDialog::begin);
        connect(&transaction, &Transaction::closed, &dialog, &QWidget::hide);
        transaction.enterCall();
        transaction.touch("Auto edit");
        QCoreApplication::processEvents();
        const bool blockedDuringCall = QApplication::activeModalWidget() == &dialog;
        transaction.leaveCall();
        QCoreApplication::processEvents();
        const bool closed = !transaction.isOpen();
        if (!closed)
            transaction.end();
        transaction.setUndoStack(nullptr);
        QVERIFY(blockedDuringCall);
        QVERIFY(closed);
        QVERIFY(!dialog.isVisible());
    }

    void emptyScriptTransaction()
    {
        QWidget window;
        EditTransactionDialog dialog(&window);
        auto &transaction = Transaction::singleton();
        QUndoStack stack;
        transaction.setUndoStack(&stack);
        connect(&transaction, &Transaction::opened, &dialog, &EditTransactionDialog::begin);
        connect(&transaction, &Transaction::closed, &dialog, &QWidget::hide);
        transaction.begin("Empty", Transaction::Origin::Script);
        const bool blocked = QApplication::activeModalWidget() == &dialog;
        const auto result = transaction.end();
        transaction.setUndoStack(nullptr);
        QVERIFY(blocked);
        QCOMPARE(result, Transaction::Result::Empty);
        QVERIFY(!dialog.isVisible());
        QCOMPARE(stack.count(), 0);
    }

    void blocksWithoutProcessingEvents()
    {
        QWidget window;
        window.show();
        EditTransactionDialog dialog(&window);
        QVERIFY(!dialog.isVisible());
        bool timerFired = false;
        QTimer::singleShot(0, &dialog, [&]() { timerFired = true; });
        dialog.begin("<b>Batch edits</b>");
        QVERIFY(!timerFired);
        QCOMPARE(QApplication::activeModalWidget(), &dialog);
        QCOMPARE(dialog.windowModality(), Qt::ApplicationModal);
        const auto *label = dialog.findChild<QLabel *>();
        QVERIFY(label);
        QCOMPARE(label->textFormat(), Qt::PlainText);
        QCOMPARE(label->text(), QStringLiteral("<b>Batch edits</b>"));
        QTest::keyClick(&dialog, Qt::Key_Escape);
        QVERIFY(dialog.isVisible());
        QVERIFY(!dialog.close());
        QVERIFY(dialog.isVisible());
        dialog.hide();
        QVERIFY(!QApplication::activeModalWidget());
        dialog.begin({});
        QVERIFY(!label->text().isEmpty());
        dialog.hide();
        QCoreApplication::processEvents();
        QVERIFY(timerFired);
    }

    void allowsCommandDialogs()
    {
        QWidget window;
        window.show();
        QDialog existing(&window);
        existing.setWindowModality(Qt::ApplicationModal);
        existing.show();
        EditTransactionDialog dialog(&window);
        dialog.begin("Editing");
        QProgressDialog progress("Command progress", QString(), 0, 0, &window);
        progress.setWindowModality(Qt::ApplicationModal);
        progress.show();
        QCOMPARE(QApplication::activeModalWidget(), &progress);
        progress.hide();
        QCOMPARE(QApplication::activeModalWidget(), &dialog);
        QDialog message(&window);
        message.setWindowModality(Qt::ApplicationModal);
        QTimer::singleShot(0, &message, &QDialog::accept);
        QCOMPARE(message.exec(), int(QDialog::Accepted));
        QCOMPARE(QApplication::activeModalWidget(), &dialog);
        dialog.hide();
        QCOMPARE(QApplication::activeModalWidget(), &existing);
        existing.hide();
        QVERIFY(!QApplication::activeModalWidget());
    }
};

QTEST_MAIN(TestEditTransactionDialog)
#include "test_edittransactiondialog.moc"
