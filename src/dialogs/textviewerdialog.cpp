/*
 * Copyright (c) 2012-2024 Meltytech, LLC
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

#include "textviewerdialog.h"
#include "ui_textviewerdialog.h"

#include "settings.h"
#include "util.h"

#include <QClipboard>
#include <QFileDialog>
#include <QPushButton>
#include <QScrollBar>

TextViewerDialog::TextViewerDialog(QWidget *parent, bool forMltXml)
    : QDialog(parent)
    , ui(new Ui::TextViewerDialog)
    , m_forMltXml(forMltXml)
{
    ui->setupUi(this);
    auto button = ui->buttonBox->addButton(tr("Copy"), QDialogButtonBox::ActionRole);
    connect(button, &QAbstractButton::clicked, this, [&]() {
        QGuiApplication::clipboard()->setText(ui->plainTextEdit->toPlainText());
    });
}

TextViewerDialog::~TextViewerDialog()
{
    delete ui;
}

void TextViewerDialog::setText(const QString &s, bool scroll)
{
    if (s != ui->plainTextEdit->toPlainText()) {
        ui->plainTextEdit->setPlainText(s);
        if (scroll)
            ui->plainTextEdit->verticalScrollBar()->setValue(
                ui->plainTextEdit->verticalScrollBar()->maximum());
    }
}

QDialogButtonBox *TextViewerDialog::buttonBox() const
{
    return ui->buttonBox;
}

void TextViewerDialog::on_buttonBox_accepted()
{
    QString path = Settings.savePath();
    QString caption = tr("Save Text");
    QString nameFilter = tr("Text Documents (*.txt);;All Files (*)");
    if (m_forMltXml) {
        nameFilter = tr("MLT XML (*.mlt);;All Files (*)");
    }
    QString filename = QFileDialog::getSaveFileName(this,
                                                    caption,
                                                    path,
                                                    nameFilter,
                                                    nullptr,
                                                    Util::getFileDialogOptions());
    if (!filename.isEmpty()) {
        QFileInfo fi(filename);
        if (fi.suffix().isEmpty()) {
            if (m_forMltXml)
                filename += ".mlt";
            else
                filename += ".txt";
        }
        if (Util::warnIfNotWritable(filename, this, caption))
            return;
        QFile f(filename);
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        f.write(ui->plainTextEdit->toPlainText().toUtf8());
        f.close();
    }
}
