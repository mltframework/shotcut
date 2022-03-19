/*
 * Copyright (c) 2022 Meltytech, LLC
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

#include "notesdock.h"

#include <Logger.h>

#include <QAction>
#include <QIcon>
#include <QTextEdit>
#include <QApplication>

NotesDock::NotesDock(QWidget *parent) :
    QDockWidget(tr("Notes"), parent),
    m_textEdit(new QTextEdit(this)),
    m_blockUpdate(false)
{
    LOG_DEBUG() << "begin";
    setObjectName("NotesDock");
    QIcon filterIcon = QIcon::fromTheme("document-edit", QIcon(":/icons/oxygen/32x32/actions/document-edit.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());

    m_textEdit->setTabChangesFocus(false);
    m_textEdit->setTabStopDistance(m_textEdit->fontMetrics().horizontalAdvance("XXXX")); // Tabstop = 4 spaces
    m_textEdit->setAcceptRichText(false);
    m_textEdit->setFontPointSize(QApplication::font("QMenu").pointSize());
    QObject::connect(m_textEdit, SIGNAL(textChanged()), SLOT(onTextChanged()));
    QDockWidget::setWidget(m_textEdit);

    LOG_DEBUG() << "end";
}

QString NotesDock::getText()
{
    return m_textEdit->toPlainText();
}

void NotesDock::setText(const QString& text)
{
    m_blockUpdate = true;
    m_textEdit->setPlainText(text);
    m_blockUpdate = false;
}

void NotesDock::onTextChanged()
{
    if (!m_blockUpdate) {
        emit modified();
    }
}
