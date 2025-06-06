/*
 * Copyright (c) 2025 Meltytech, LLC
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

#ifndef FILEDOWNLOADDIALOG_H
#define FILEDOWNLOADDIALOG_H

#include <QProgressDialog>

class QFile;
class QNetworkReply;

class FileDownloadDialog : public QProgressDialog
{
public:
    explicit FileDownloadDialog(const QString &title, QWidget *parent = nullptr);
    ~FileDownloadDialog();
    void setSrc(const QString &src);
    void setDst(const QString &dst);
    bool start();
private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onReadyRead();
    void onFinished();

private:
    QString m_src;
    QString m_dst;
    QFile *m_file;
    QNetworkReply *m_reply;
    int m_replyCode;
};

#endif // FILEDOWNLOADDIALOG_H
