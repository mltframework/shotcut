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
#ifndef MESSAGEDIALOG_H
#define MESSAGEDIALOG_H

#include <QMessageBox>

class MessageDialog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int buttons READ buttons WRITE setButtons NOTIFY buttonsChanged)

public:
    enum StandardButtons {
        Ok                 = QMessageBox::Ok,
        Yes                = QMessageBox::Yes,
        No                 = QMessageBox::No,
        Cancel             = QMessageBox::Cancel
    };
    Q_ENUM(StandardButtons)
    explicit MessageDialog(QObject *parent = nullptr);

    Q_INVOKABLE void open();

signals:
    void titleChanged(const QString &title);
    void textChanged(const QString &text);
    void buttonsChanged(int buttons);
    void accepted();
    void rejected();

private:
    QString m_title;
    QString m_text;
    int m_buttons;

    QString title() const
    {
        return m_title;
    }
    void setTitle(const QString &title);
    QString text() const
    {
        return m_text;
    }
    void setText(const QString &text);
    int buttons() const
    {
        return m_buttons;
    }
    void setButtons(int buttons);
};

#endif // MESSAGEDIALOG_H
