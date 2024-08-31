/*
 * Copyright (c) 2022-2024 Meltytech, LLC
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
#include "actions.h"
#include "settings.h"

#include <Logger.h>

#include <QAction>
#include <QIcon>
#include <QPlainTextEdit>
#include <QApplication>
#include <QMenu>
#include <QWheelEvent>

class TextEditor: public QPlainTextEdit
{
public:
    explicit TextEditor(QWidget *parent = nullptr) : QPlainTextEdit()
    {
        zoomIn(Settings.notesZoom());
        setTabChangesFocus(false);
        setTabStopDistance(fontMetrics().horizontalAdvance("XXXX")); // Tabstop = 4 spaces
        setContextMenuPolicy(Qt::CustomContextMenu);
        auto action = new QAction(tr("Decrease Text Size"), this);
        action->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_Minus);
        addAction(action);
        Actions.add("notesDecreaseTextSize", action, tr("Notes"));
        connect(action, &QAction::triggered, this, [ = ]() {
            setZoom(-4);
        });
        action = new QAction(tr("Increase Text Size"), this);
        action->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_Equal);
        addAction(action);
        Actions.add("notesIncreaseTextSize", action, tr("Notes"));
        connect(action, &QAction::triggered, this, [ = ]() {
            setZoom(4);
        });
        connect(this, &QWidget::customContextMenuRequested, this, [ = ](const QPoint & pos) {
            std::unique_ptr<QMenu> menu {createStandardContextMenu()};
            actions().at(0)->setEnabled(Settings.notesZoom() > 0);
            menu->addActions(actions());
            menu->exec(mapToGlobal(pos));
        });
    }

    void setZoom(int delta)
    {
        auto zoom = Settings.notesZoom();
        zoomIn((zoom + delta >= 0) ? delta : -zoom);
        Settings.setNotesZoom(std::max<int>(0, zoom + delta));
    }

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (event->modifiers() & Qt::ControlModifier) {
            setZoom((event->angleDelta().y() < 0) ? -1 : 1);
            event->accept();
        } else {
            QPlainTextEdit::wheelEvent(event);
        }
    }
};

NotesDock::NotesDock(QWidget *parent) :
    QDockWidget(tr("Notes"), parent),
    m_textEdit(new TextEditor(this)),
    m_blockUpdate(false)
{
    LOG_DEBUG() << "begin";
    setObjectName("NotesDock");
    QIcon filterIcon = QIcon::fromTheme("document-edit",
                                        QIcon(":/icons/oxygen/32x32/actions/document-edit.png"));
    setWindowIcon(filterIcon);
    toggleViewAction()->setIcon(windowIcon());

    QObject::connect(m_textEdit, SIGNAL(textChanged()), SLOT(onTextChanged()));
    QDockWidget::setWidget(m_textEdit);

    LOG_DEBUG() << "end";
}

QString NotesDock::getText()
{
    return m_textEdit->toPlainText();
}

void NotesDock::setText(const QString &text)
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
