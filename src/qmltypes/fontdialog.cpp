/*
 * Copyright (c) 2023-2026 Meltytech, LLC
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

#include "fontdialog.h"

#include <QFontDialog>
#include <QOperatingSystemVersion>

FontDialog::FontDialog(QObject *parent)
    : QObject{parent}
{}

void FontDialog::open()
{
    QFontDialog dialog(m_font);
    dialog.setModal(true);
#if defined(Q_OS_MAC)
    // macOS 27's font panel has no OK or Cancel button.
    if (QOperatingSystemVersion::current()
        >= QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 27)) {
        dialog.setOption(QFontDialog::DontUseNativeDialog);
    }
#elif defined(Q_OS_UNIX)
    dialog.setOption(QFontDialog::DontUseNativeDialog);
#endif
    if (dialog.exec() == QDialog::Accepted) {
        setSelectedFont(dialog.currentFont());
        emit accepted();
    } else {
        emit rejected();
    }
}

void FontDialog::setSelectedFont(const QFont &font)
{
    if (font != m_font) {
        m_font = font;
        emit selectedFontChanged(font);
    }
}
