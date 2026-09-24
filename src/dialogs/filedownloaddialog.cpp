/*
 * Copyright (c) 2025-2026 Meltytech, LLC
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
#include "util.h"

#include <QCryptographicHash>
#include <QFile>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

static const int PROGRESS_MAX = 1000;

static bool isSha256Hex(const QString &digest)
{
    if (digest.size() != 64)
        return false;
    for (const QChar &c : digest) {
        const char16_t u = c.unicode();
        const bool hex = (u >= u'0' && u <= u'9') || (u >= u'a' && u <= u'f')
                         || (u >= u'A' && u <= u'F');
        if (!hex)
            return false;
    }
    return true;
}

FileDownloadDialog::FileDownloadDialog(const QString &title, QWidget *parent)
    : QProgressDialog(title, tr("Cancel"), 0, PROGRESS_MAX, parent ? parent : &MAIN)
{
    setWindowTitle(title);
    setModal(true);
    setWindowModality(Qt::ApplicationModal);
    setMinimumDuration(0);
}

bool FileDownloadDialog::start(const QString &url,
                               const QString &destination,
                               const QString &sha256,
                               QStringView host)
{
    LOG_INFO() << "Download Source" << url;
    LOG_INFO() << "Download Destination" << destination;
    if (destination.isEmpty() || !isSha256Hex(sha256) || !Util::isHttpsOnHost(url, host)) {
        LOG_ERROR() << "Refusing download";
        QMessageBox::information(this, windowTitle(), tr("Download Failed"));
        return false;
    }

    QFile file(destination + QStringLiteral(".tmp"));
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR() << "Unable to open file to write";
        QMessageBox::information(this, windowTitle(), tr("Download Failed"));
        return false;
    }

    QCryptographicHash hasher(QCryptographicHash::Sha256);
    bool writeFailed = false;
    QNetworkAccessManager manager(this);
    QNetworkRequest request{QUrl(url, QUrl::StrictMode)};
    request.setTransferTimeout(6000);
    // HTTPS redirects stay on HTTPS. Hugging Face resolve URLs redirect to a CDN.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = manager.get(request);

    QObject::connect(reply,
                     &QNetworkReply::downloadProgress,
                     this,
                     [this](qint64 bytesReceived, qint64 bytesTotal) {
                         if (bytesTotal > 0) {
                             int progress = bytesReceived * PROGRESS_MAX / bytesTotal;
                             LOG_INFO() << "Download Progress" << progress / 10;
                             setValue(progress);
                         }
                     });
    QObject::connect(reply, &QNetworkReply::readyRead, this, [&]() {
        if (writeFailed)
            return;
        const QByteArray data = reply->readAll();
        if (file.write(data) != data.size()) {
            LOG_ERROR() << "Short write";
            writeFailed = true;
            reply->abort();
            return;
        }
        hasher.addData(data);
    });
    QObject::connect(reply, &QNetworkReply::finished, this, &QDialog::accept);

    const int result = exec();
    if (result != QDialog::Accepted) {
        LOG_WARNING() << "Download canceled";
        file.remove();
        return false;
    }
    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR() << reply->errorString();
        file.remove();
        QMessageBox::information(this, windowTitle(), tr("Download Failed"));
        return false;
    }

    file.flush();
    file.close();
    const QString actual = QString::fromLatin1(hasher.result().toHex());
    if (actual.compare(sha256, Qt::CaseInsensitive) != 0) {
        LOG_ERROR() << "SHA-256 verification failed";
        file.remove();
        QMessageBox::information(this,
                                 windowTitle(),
                                 tr("The downloaded file failed verification."));
        return false;
    }
    if (!file.rename(destination)) {
        LOG_ERROR() << "Unable to rename download";
        file.remove();
        QMessageBox::information(this, windowTitle(), tr("Download Failed"));
        return false;
    }
    return true;
}
