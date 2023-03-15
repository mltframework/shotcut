/*
 * Copyright (c) 2023 Meltytech, LLC
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

#include "messagedialog.h"
#include "qmlapplication.h"
#include <Logger.h>

#include <QApplication>

MessageDialog::MessageDialog(QObject *parent)
    : QObject{parent}
    , m_buttons{0}
{
}

void MessageDialog::open()
{
    QMessageBox dialog;
    if (m_buttons & QMessageBox::No) {
        dialog.setIcon(QMessageBox::Question);
        dialog.setStandardButtons(QMessageBox::StandardButtons(m_buttons));
        dialog.setDefaultButton(QMessageBox::Yes);
        dialog.setEscapeButton(QMessageBox::No);
    } else if (!m_buttons) {
        dialog.setIcon(QMessageBox::Information);
        dialog.setDefaultButton(QMessageBox::Ok);
    } else {
        dialog.setStandardButtons(QMessageBox::StandardButtons(m_buttons));
    }
    if (!m_title.isEmpty()) {
        dialog.setWindowTitle(m_title);
    } else {
        dialog.setWindowTitle(QApplication::applicationName());
    }
    dialog.setText(m_text);
    dialog.setWindowModality(QmlApplication::dialogModality());
    auto button = QMessageBox::StandardButton(dialog.exec());
    if (QMessageBox::Ok == button || QMessageBox::Yes == button) {
        emit accepted();
    } else {
        emit rejected();
    }
}

void MessageDialog::setTitle(const QString &title)
{
    if (title != m_title) {
        m_title = title;
        emit titleChanged(title);
    }
}

void MessageDialog::setText(const QString &text)
{
    if (text != m_text) {
        m_text = text;
        emit textChanged(text);
    }
}

void MessageDialog::setButtons(int buttons)
{
    if (buttons != m_buttons) {
        m_buttons = buttons;
        emit buttonsChanged(buttons);
    }
}
