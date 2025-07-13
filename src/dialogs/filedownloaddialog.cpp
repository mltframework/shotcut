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

#include "filedownloaddialog.h"

#include "Logger.h"
#include "mainwindow.h"
#include "qmltypes/qmlapplication.h"

#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

static const int PROGRESS_MAX = 1000;

FileDownloadDialog::FileDownloadDialog(const QString &title, QWidget *parent)
    : QProgressDialog(title, tr("Cancel"), 0, PROGRESS_MAX, parent ? parent : &MAIN)
{
    setWindowTitle(title);
    setModal(true);
    setWindowModality(Qt::ApplicationModal);
    setMinimumDuration(0);
}

FileDownloadDialog::~FileDownloadDialog() {}

void FileDownloadDialog::setSrc(const QString &src)
{
    m_src = src;
}

void FileDownloadDialog::setDst(const QString &dst)
{
    m_dst = dst;
}

bool FileDownloadDialog::start()
{
    LOG_INFO() << "Download Source" << m_src;
    LOG_INFO() << "Download Destination" << m_dst;
    bool retVal = false;
    QString tmpPath = m_dst + ".tmp";
    m_file = new QFile(tmpPath, this);
    if (!m_file || !m_file->open(QIODevice::WriteOnly)) {
        LOG_ERROR() << "Unable to open file to write";
        delete m_file;
        return retVal;
    }

    QNetworkAccessManager manager(this);
    QUrl url = m_src;
    QNetworkRequest request(url);
    request.setTransferTimeout(6000);
    m_reply = manager.get(request);

    QObject::connect(m_reply,
                     &QNetworkReply::downloadProgress,
                     this,
                     &FileDownloadDialog::onDownloadProgress);
    QObject::connect(m_reply, &QNetworkReply::readyRead, this, &FileDownloadDialog::onReadyRead);
    QObject::connect(m_reply, &QNetworkReply::finished, this, &FileDownloadDialog::onFinished);
    QObject::connect(m_reply, &QNetworkReply::sslErrors, this, &FileDownloadDialog::sslErrors);

    int result = exec();
    if (result != QDialog::Accepted) {
        LOG_WARNING() << "Download canceled";
        m_file->remove();
    } else if (m_reply->error() != QNetworkReply::NoError) {
        LOG_ERROR() << m_reply->errorString();
        if (m_reply->error() == QNetworkReply::UnknownNetworkError && m_src.startsWith("https:")) {
            m_src.replace("https://", "http://");
            return start();
        }
        if (m_reply->error() != QNetworkReply::NoError) {
            m_file->remove();
            QMessageBox::information(this, windowTitle(), tr("Download Failed"));
        }
    } else {
        // Notify success
        m_file->rename(m_dst);
        retVal = true;
    }
    delete m_reply;
    delete m_file;
    return retVal;
}

void FileDownloadDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = bytesReceived * PROGRESS_MAX / bytesTotal;
        LOG_INFO() << "Download Progress" << progress / 10;
        setValue(progress);
    }
}

void FileDownloadDialog::onReadyRead()
{
    m_file->write(m_reply->readAll());
}

void FileDownloadDialog::onFinished()
{
    accept();
}

void FileDownloadDialog::sslErrors(const QList<QSslError> &errors)
{
    LOG_ERROR() << "SSL Errors" << errors;
    QString message = tr("The following SSL errors were encountered:");
    foreach (const QSslError &error, errors) {
        message = QStringLiteral("\n") + error.errorString();
    }
    message += tr("Attempt to ignore SSL errors?");
    QMessageBox qDialog(QMessageBox::Question,
                        windowTitle(),
                        message,
                        QMessageBox::No | QMessageBox::Yes,
                        this);
    qDialog.setDefaultButton(QMessageBox::Yes);
    qDialog.setEscapeButton(QMessageBox::No);
    qDialog.setWindowModality(QmlApplication::dialogModality());
    int result = qDialog.exec();
    if (result == QMessageBox::Yes) {
        m_reply->ignoreSslErrors();
    }
}
