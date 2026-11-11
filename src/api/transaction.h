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

#ifndef SHOTCUT_API_TRANSACTION_H
#define SHOTCUT_API_TRANSACTION_H

#include <QObject>
#include <QString>

class QUndoCommand;
class QUndoStack;

namespace Shotcut {
namespace Api {

// Groups all undo commands pushed by API calls into a single undo entry.
//
// Transactions nest; only the outermost begin()/end() pair opens and closes
// the QUndoStack macro. A failure anywhere in the nest rolls back the whole
// transaction when the outermost level ends, leaving no undo or redo entry.
//
// touch() starts an "auto" transaction if none is open. It closes when control
// returns to the event loop, so a burst of synchronous calls (e.g., one QML
// button click) becomes one undo entry without any explicit begin/end.
class Transaction : public QObject
{
    Q_OBJECT

public:
    enum class Origin { Coproducer, Script };
    enum class Result { Committed, RolledBack, Empty, Nested, NotOpen };
    Q_ENUM(Result)

    static Transaction &singleton();

    void begin(const QString &text = QString(), Origin origin = Origin::Coproducer);
    Result end();
    void touch(const QString &text = QString());
    void fail(const QString &reason);

    // Brackets one API call. An auto transaction never closes while a call is
    // in progress, even when a command spins a nested event loop (for example
    // LongUiTask::reportProgress() calls QCoreApplication::processEvents()).
    void enterCall() { ++m_callDepth; }
    void leaveCall();

    // True from begin() until the outermost end() has finished committing or rolling back.
    bool isOpen() const { return m_depth > 0 || m_closing; }
    bool isAuto() const { return m_auto; }
    bool hasFailed() const { return m_failed; }
    bool isScriptMacro() const { return m_scriptMacro; }
    int depth() const { return m_depth; }
    QString text() const { return m_text; }
    QString failureReason() const { return m_failureReason; }

    // Overrides the undo stack, for testing. Defaults to MAIN.undoStack().
    void setUndoStack(QUndoStack *stack) { m_stack = stack; }

signals:
    void opened(const QString &text);
    void closed(const QString &text, Shotcut::Api::Transaction::Result result);

private:
    explicit Transaction(QObject *parent = nullptr);
    QUndoStack *undoStack() const;
    void openMacro();
    void onAutoCloseDue();
    Result closeOutermost();
    void reset();

    QUndoStack *m_stack = nullptr;
    QUndoCommand *m_macro = nullptr;
    int m_depth = 0;
    int m_callDepth = 0;
    bool m_auto = false;
    bool m_scriptMacro = false;
    bool m_autoCloseDue = false;
    bool m_autoCloseWaitingForCall = false;
    bool m_failed = false;
    bool m_closing = false;
    QString m_text;
    QString m_failureReason;
};

// RAII helper: begins a transaction on construction and ends it on
// destruction unless end() was already called.
class TransactionScope
{
public:
    explicit TransactionScope(const QString &text = QString());
    ~TransactionScope();
    Q_DISABLE_COPY(TransactionScope)

    void fail(const QString &reason);
    Transaction::Result end();

private:
    bool m_ended = false;
};

class ApiCallGuard
{
public:
    ApiCallGuard() { Transaction::singleton().enterCall(); }
    ~ApiCallGuard() { Transaction::singleton().leaveCall(); }
    Q_DISABLE_COPY(ApiCallGuard)
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_TRANSACTION_H
