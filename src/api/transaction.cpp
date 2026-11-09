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

#include "Logger.h"
#include "mainwindow.h"

#include <QTimer>
#include <QUndoCommand>
#include <QUndoStack>

namespace Shotcut {
namespace Api {

Transaction::Transaction(QObject *parent)
    : QObject(parent)
{}

Transaction &Transaction::singleton()
{
    static Transaction instance;
    return instance;
}

QUndoStack *Transaction::undoStack() const
{
    return m_stack ? m_stack : MAIN.undoStack();
}

void Transaction::openMacro()
{
    auto *stack = undoStack();
    stack->beginMacro(m_text);
    m_macro = const_cast<QUndoCommand *>(stack->command(stack->count() - 1));
}

void Transaction::begin(const QString &text, Origin origin)
{
    if (origin == Origin::Script)
        m_scriptMacro = true;
    if (m_depth == 0) {
        m_text = text;
        m_failed = false;
        m_failureReason.clear();
        // Script callbacks can be empty or fail before editing; preserve redo in that case.
        if (origin != Origin::Script)
            openMacro();
        LOG_DEBUG() << "begin" << text << (m_auto ? "(auto)" : "");
        ++m_depth;
        emit opened(text);
        return;
    }
    if (m_text.isEmpty() && !text.isEmpty())
        m_text = text;
    ++m_depth;
}

Transaction::Result Transaction::end()
{
    if (m_depth == 0) {
        LOG_WARNING() << "end() called without an open transaction";
        return Result::NotOpen;
    }
    --m_depth;
    if (m_depth == 0)
        return closeOutermost();
    if (m_depth == 1 && m_auto && m_autoCloseDue)
        return end();
    return Result::Nested;
}

void Transaction::touch(const QString &text)
{
    if (m_depth > 0) {
        if (!m_macro)
            openMacro();
        return;
    }
    m_auto = true;
    m_autoCloseDue = false;
    begin(text);
    QTimer::singleShot(0, this, &Transaction::onAutoCloseDue);
}

void Transaction::fail(const QString &reason)
{
    if (m_depth == 0) {
        LOG_WARNING() << "fail() called without an open transaction:" << reason;
        return;
    }
    if (!m_failed) {
        LOG_DEBUG() << "fail" << reason;
        m_failureReason = reason;
    }
    m_failed = true;
}

void Transaction::leaveCall()
{
    if (m_callDepth > 0)
        --m_callDepth;
    if (m_callDepth == 0 && m_autoCloseWaitingForCall) {
        m_autoCloseWaitingForCall = false;
        QTimer::singleShot(0, this, &Transaction::onAutoCloseDue);
    }
}

void Transaction::onAutoCloseDue()
{
    if (!m_auto || m_depth == 0)
        return;
    if (m_callDepth > 0) {
        m_autoCloseWaitingForCall = true;
        return;
    }
    // An explicit begin() nested inside the auto transaction is still open,
    // for example across an asynchronous step; close when it ends.
    if (m_depth > 1) {
        m_autoCloseDue = true;
        return;
    }
    end();
}

Transaction::Result Transaction::closeOutermost()
{
    m_closing = true;
    auto *stack = undoStack();
    auto *macro = m_macro;
    const QString text = m_text;
    Result result = Result::Committed;
    if (!macro) {
        result = m_failed ? Result::RolledBack : Result::Empty;
        reset();
        emit closed(text, result);
        return result;
    }

    if (macro && !m_failed) {
        QString label = m_text;
        if (label.isEmpty()) {
            const int n = macro->childCount();
            label = n == 1 ? macro->child(0)->text() : tr("%n operation(s)", nullptr, n);
        }
        macro->setText(
            (m_auto || m_scriptMacro ? tr("Script: %1") : tr("Coproducer: %1")).arg(label));
    }

    stack->endMacro();

    if (!macro || stack->index() < 1 || stack->command(stack->index() - 1) != macro) {
        LOG_ERROR() << "transaction macro not found on top of the undo stack";
    } else if (m_failed || macro->childCount() == 0) {
        // QUndoStack::undo() skips undo() for an obsolete command and then
        // deletes it, so undo the children here and let the stack drop the
        // macro. This leaves neither an undo nor a redo entry.
        if (m_failed) {
            macro->undo();
            result = Result::RolledBack;
        } else {
            result = Result::Empty;
        }
        macro->setObsolete(true);
        stack->undo();
    }

    LOG_DEBUG() << "end" << text << result << m_failureReason;
    reset();
    emit closed(text, result);
    return result;
}

void Transaction::reset()
{
    m_macro = nullptr;
    m_depth = 0;
    m_auto = false;
    m_scriptMacro = false;
    m_autoCloseDue = false;
    m_autoCloseWaitingForCall = false;
    m_failed = false;
    m_closing = false;
    m_text.clear();
}

TransactionScope::TransactionScope(const QString &text)
{
    Transaction::singleton().begin(text);
}

TransactionScope::~TransactionScope()
{
    if (!m_ended)
        end();
}

void TransactionScope::fail(const QString &reason)
{
    Transaction::singleton().fail(reason);
}

Transaction::Result TransactionScope::end()
{
    if (m_ended)
        return Transaction::Result::NotOpen;
    m_ended = true;
    return Transaction::singleton().end();
}

} // namespace Api
} // namespace Shotcut
