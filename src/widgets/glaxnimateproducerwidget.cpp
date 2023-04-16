/*
 * Copyright (c) 2022-2023 Meltytech, LLC
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

#include <QColorDialog>
#include <QFileInfo>
#include "glaxnimateproducerwidget.h"
#include "mainwindow.h"
#include "settings.h"
#include "ui_glaxnimateproducerwidget.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"
#include "dialogs/longuitask.h"
#include "videowidget.h"
#include <Logger.h>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrentRun>
#include <QFuture>

static const QString kTransparent = QObject::tr("transparent", "Open Other > Animation");

static QString colorToString(const QColor &color)
{
    return (color == QColor(0, 0, 0, 0)) ? kTransparent
           : QString::asprintf("#%02X%02X%02X%02X",
                               qAlpha(color.rgba()),
                               qRed(color.rgba()),
                               qGreen(color.rgba()),
                               qBlue(color.rgba()));
}

static QString colorStringToResource(const QString &s)
{
    return (s == kTransparent) ? "#00000000" : s;
}

GlaxnimateProducerWidget::GlaxnimateProducerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GlaxnimateProducerWidget)
{
    ui->setupUi(this);
    m_title = ui->lineEdit->text();
    ui->colorLabel->setText(kTransparent);
    Util::setColorsToHighlight(ui->lineEdit, QPalette::Base);
    ui->preset->saveDefaultPreset(getPreset());
    Mlt::Properties p;
    p.set("background", "#FF000000");
    ui->preset->savePreset(p, tr("black"));
    p.set("background", "#00000000");
    ui->preset->savePreset(p, tr("transparent"));
    ui->preset->loadPresets();
    ui->notesLabel->setVisible(false);
    ui->notesTextEdit->setVisible(false);
    ui->editButton->setVisible(false);
    ui->reloadButton->setVisible(false);
    ui->durationSpinBox->setValue(qRound(5.0 * MLT.profile().fps()));
}

GlaxnimateProducerWidget::~GlaxnimateProducerWidget()
{
    GlaxnimateIpcServer::instance().reset();
    delete ui;
}

void GlaxnimateProducerWidget::on_colorButton_clicked()
{
    QColor color = colorStringToResource(ui->colorLabel->text());
    if (m_producer) {
        color = QColor(QFileInfo(m_producer->get("background")).baseName());
    }
    QColorDialog dialog(color);
    dialog.setOption(QColorDialog::ShowAlphaChannel);
    dialog.setModal(QmlApplication::dialogModality());
    if (dialog.exec() == QDialog::Accepted) {
        auto newColor = dialog.currentColor();
        auto rgb = newColor;
        auto transparent = QColor(0, 0, 0, 0);
        rgb.setAlpha(color.alpha());
        if (newColor.alpha() == 0 && (rgb != color ||
                                      (newColor == transparent && color == transparent))) {
            newColor.setAlpha(255);
        }
        ui->colorLabel->setText(colorToString(newColor));
        ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
                                      .arg(Util::textColor(newColor), newColor.name()));
        if (m_producer) {
            m_producer->set("background", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
            emit producerChanged(m_producer.get());
        }
    }
}

static void modifyJsonValue(QJsonValue &destValue, const QString &path, const QJsonValue &newValue)
{
    const int indexOfDot = path.indexOf('.');
    const QString dotPropertyName = path.left(indexOfDot);
    const QString dotSubPath = indexOfDot > 0 ? path.mid(indexOfDot + 1) : QString();

    const int indexOfSquareBracketOpen = path.indexOf('[');
    const int indexOfSquareBracketClose = path.indexOf(']');

    const int arrayIndex = path.mid(indexOfSquareBracketOpen + 1,
                                    indexOfSquareBracketClose - indexOfSquareBracketOpen - 1).toInt();

    const QString squareBracketPropertyName = path.left(indexOfSquareBracketOpen);
    const QString squareBracketSubPath = indexOfSquareBracketClose > 0 ? (path.mid(
                                                                              indexOfSquareBracketClose + 1)[0] == '.' ? path.mid(indexOfSquareBracketClose + 2) : path.mid(
                                                                              indexOfSquareBracketClose + 1)) : QString();

    // determine what is first in path. dot or bracket
    bool useDot = true;
    if (indexOfDot >= 0) { // there is a dot in path
        if (indexOfSquareBracketOpen >= 0) { // there is squarebracket in path
            if (indexOfDot > indexOfSquareBracketOpen)
                useDot = false;
            else
                useDot = true;
        } else {
            useDot = true;
        }
    } else {
        if (indexOfSquareBracketOpen >= 0)
            useDot = false;
        else
            useDot = true; // actually, id doesn't matter, both dot and square bracket don't exist
    }

    QString usedPropertyName = useDot ? dotPropertyName : squareBracketPropertyName;
    QString usedSubPath = useDot ? dotSubPath : squareBracketSubPath;

    QJsonValue subValue;
    if (destValue.isArray()) {
        subValue = destValue.toArray()[usedPropertyName.toInt()];
    } else if (destValue.isObject()) {
        subValue = destValue.toObject()[usedPropertyName];
    } else {
        LOG_WARNING() << "unable to handle" << destValue;
    }

    if (usedSubPath.isEmpty()) {
        subValue = newValue;
    } else {
        if (subValue.isArray()) {
            QJsonArray arr = subValue.toArray();
            QJsonValue arrEntry = arr[arrayIndex];
            modifyJsonValue(arrEntry, usedSubPath, newValue);
            arr[arrayIndex] = arrEntry;
            subValue = arr;
        } else if (subValue.isObject()) {
            modifyJsonValue(subValue, usedSubPath, newValue);
        } else {
            subValue = newValue;
        }
    }

    if (destValue.isArray()) {
        QJsonArray arr = destValue.toArray();
        arr[arrayIndex] = subValue;
        destValue = arr;
    } else if (destValue.isObject()) {
        QJsonObject obj = destValue.toObject();
        obj[usedPropertyName] = subValue;
        destValue = obj;
    } else {
        destValue = newValue;
    }
}


Mlt::Producer *GlaxnimateProducerWidget::newProducer(Mlt::Profile &profile)
{
    // Get the file name.
    auto filename = QmlApplication::getNextProjectFile("anim.rawr");
    if (filename.isEmpty()) {
        QString path = Settings.savePath();
        path.append("/%1.rawr");
        path = path.arg(tr("animation"));
        auto nameFilter = tr("Glaxnimate (*.rawr);;All Files (*)");
        filename = QFileDialog::getSaveFileName(this, tr("New Animation"), path, nameFilter,
                                                nullptr, Util::getFileDialogOptions());
    }
    if (filename.isEmpty()) {
        return nullptr;
    }
    if (!filename.endsWith(".rawr")) {
        filename += ".rawr";
    }
    auto info = QFileInfo(filename);
    Settings.setSavePath(info.path());

    GlaxnimateIpcServer::instance().newFile(filename, ui->durationSpinBox->value());

    Mlt::Producer *p = new Mlt::Producer(profile,
                                         QString("glaxnimate:").append(filename).toUtf8().constData());
    p->set("background", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());

    m_title = info.fileName();
    p->set(kShotcutCaptionProperty, m_title.toUtf8().constData());
    p->set(kShotcutDetailProperty, filename.toUtf8().constData());

    m_watcher.reset(new QFileSystemWatcher({filename}));
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this,
            &GlaxnimateProducerWidget::onFileChanged);
    GlaxnimateIpcServer::instance().launch(p);

    return p;
}

void GlaxnimateProducerWidget::setProducer(Mlt::Producer *p)
{
    AbstractProducerWidget::setProducer(p);
    ui->notesLabel->setVisible(true);
    ui->notesTextEdit->setVisible(true);
    ui->notesTextEdit->setPlainText(QString::fromUtf8(p->get(kCommentProperty)));
    ui->editButton->setVisible(true);
    ui->reloadButton->setVisible(true);

    auto filename = QString::fromUtf8(p->get("resource"));
    m_title = QFileInfo(filename).fileName();
    if (QString::fromUtf8(p->get(kShotcutCaptionProperty)).isEmpty()) {
        p->set(kShotcutCaptionProperty, m_title.toUtf8().constData());
    }
    ui->lineEdit->setText(QString::fromUtf8(p->get(kShotcutCaptionProperty)));
    ui->durationSpinBox->setValue(p->get_length());

    m_watcher.reset(new QFileSystemWatcher({filename}));
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this,
            &GlaxnimateProducerWidget::onFileChanged);
}

Mlt::Properties GlaxnimateProducerWidget::getPreset() const
{
    Mlt::Properties p;
    QString color = colorStringToResource(ui->colorLabel->text());
    p.set("background", color.toLatin1().constData());
    return p;
}

void GlaxnimateProducerWidget::loadPreset(Mlt::Properties &p)
{
    QColor color(QFileInfo(p.get("background")).baseName());
    ui->colorLabel->setText(colorToString(color));
    ui->colorLabel->setStyleSheet(QString("color: %1; background-color: %2")
                                  .arg(Util::textColor(color), color.name()));
    if (m_producer) {
        m_producer->set("background", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());
        emit producerChanged(m_producer.get());
    }
}

void GlaxnimateProducerWidget::rename()
{
    ui->lineEdit->setFocus();
    ui->lineEdit->selectAll();
}

void GlaxnimateProducerWidget::on_preset_selected(void *p)
{
    Mlt::Properties *properties = (Mlt::Properties *) p;
    loadPreset(*properties);
    delete properties;
}

void GlaxnimateProducerWidget::on_preset_saveClicked()
{
    ui->preset->savePreset(getPreset());
}

void GlaxnimateProducerWidget::on_lineEdit_editingFinished()
{
    if (m_producer) {
        auto caption = ui->lineEdit->text();
        if (caption.isEmpty()) {
            caption = m_title;
            ui->lineEdit->setText(m_title);
        }
        m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
        emit modified();
    }
}

void GlaxnimateProducerWidget::on_notesTextEdit_textChanged()
{
    if (m_producer && m_producer->is_valid()) {
        QString existing = QString::fromUtf8(m_producer->get(kCommentProperty));
        if (ui->notesTextEdit->toPlainText() != existing) {
            m_producer->set(kCommentProperty, ui->notesTextEdit->toPlainText().toUtf8().constData());
            emit modified();
        }
    }
}

void GlaxnimateProducerWidget::on_editButton_clicked()
{
    if (m_producer && m_producer->is_valid()) {
        GlaxnimateIpcServer::instance().launch(*producer());
    }
}

void GlaxnimateProducerWidget::onFileChanged(const QString &path)
{
    if (!m_watcher->files().contains(path)) {
        m_watcher->addPath(path);
    }
    if (m_producer && m_producer->is_valid()) {
        m_producer->set("resource", path.toUtf8().constData());
        auto caption = QFileInfo(path).fileName();
        if (QString::fromUtf8(m_producer->get(kShotcutCaptionProperty)) == m_title) {
            m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
        }
        m_title = caption;
        m_producer->set("refresh", 1);
        MLT.refreshConsumer();
        QTimer::singleShot(1000, this, [ = ]() {
            if (ui->durationSpinBox->value() != m_producer->get_length()) {
                ui->durationSpinBox->setValue(m_producer->get_length());
                on_durationSpinBox_editingFinished();
            }
        });
    }
}

void GlaxnimateProducerWidget::on_reloadButton_clicked()
{
    if (m_producer && m_producer->is_valid()) {
        m_producer->set("refresh", 1);
        MLT.refreshConsumer();
        QTimer::singleShot(1000, [this]() {
            if (ui->durationSpinBox->value() != m_producer->get_length()) {
                ui->durationSpinBox->setValue(m_producer->get_length());
                on_durationSpinBox_editingFinished();
            }
        });
    }
}

void GlaxnimateProducerWidget::on_durationSpinBox_editingFinished()
{
    if (m_producer && m_producer->is_valid()) {
        m_producer->set("length", m_producer->frames_to_time(ui->durationSpinBox->value(), mlt_time_clock));
        emit producerChanged(m_producer.get());
    }
}

void GlaxnimateIpcServer::ParentResources::setProducer(const Mlt::Producer &producer,
                                                       bool hideCurrentTrack)
{
    m_producer = producer;
    if (!m_producer.get(kMultitrackItemProperty) && !m_producer.get(kTrackIndexProperty))
        return;
    m_profile.reset(new Mlt::Profile(::mlt_profile_clone(MLT.profile().get_profile())));
    m_profile->set_progressive(Settings.playerProgressive());
    m_glaxnimateProducer.reset(new Mlt::Producer(*m_profile, "xml-string",
                                                 MLT.XML().toUtf8().constData()));
    if (m_glaxnimateProducer && m_glaxnimateProducer->is_valid()) {
        m_frameNum = -1;
        // hide this clip's video track and upper ones
        int trackIndex = m_producer.get_int(kTrackIndexProperty);
        QString s = QString::fromLatin1(m_producer.get(kMultitrackItemProperty));
        auto parts = s.split(':');
        if (parts.length() == 2) {
            trackIndex = parts[1].toInt();
        }
        if (hideCurrentTrack && trackIndex == MAIN.bottomVideoTrackIndex()) {
            // Disable preview in Glaxnimate
            m_glaxnimateProducer.reset();
            m_profile.reset();
            GlaxnimateIpcServer::instance().copyToShared(QImage());
            return;
        }
        auto offset = hideCurrentTrack ? 1 : 0;
        Mlt::Tractor tractor(*m_glaxnimateProducer);
        // for each upper video track plus this one
        for (int i = 0; i < trackIndex + offset; i++) {
            // get the MLT track index
            auto index = MAIN.mltIndexForTrack(i);
            // get the MLT track in this copy
            std::unique_ptr<Mlt::Producer> track(tractor.track(index));
            if (track && track->is_valid()) {
                // hide the track
                track->set("hide", 3);
            }
        }

        // Disable the glaxnimate filter and below
        if (!hideCurrentTrack) {
            std::unique_ptr<Mlt::Producer> track(tractor.track(MAIN.mltIndexForTrack(trackIndex)));
            if (track && track->is_valid()) {
                auto clipIndex = parts[0].toInt();
                Mlt::Playlist playlist(*track);
                std::unique_ptr<Mlt::ClipInfo> info(playlist.clip_info(clipIndex));
                if (info && info->producer && info->producer->is_valid()) {
                    auto count = info->producer->filter_count();
                    bool found = false;
                    for (int i = 0; i < count; i++) {
                        std::unique_ptr<Mlt::Filter> filter(info->producer->filter(i));
                        if (filter && filter->is_valid()) {
                            if (found || !qstrcmp(filter->get(kShotcutFilterProperty), "maskGlaxnimate")) {
                                found = true;
                                filter->set("disable", 1);
                            }
                        }
                    }
                }
            }
        }
    }
}

void GlaxnimateIpcServer::onConnect()
{
    LOG_DEBUG() << "";
    m_socket = m_server->nextPendingConnection();
    connect(m_socket.data(), &QLocalSocket::readyRead, this, &GlaxnimateIpcServer::onReadyRead);
    connect(m_socket.data(), &QLocalSocket::errorOccurred, this, &GlaxnimateIpcServer::onSocketError);
    m_stream.reset(new QDataStream(m_socket.data()));
    m_stream->setVersion(QDataStream::Qt_5_15);
    *m_stream << QString("hello");
    m_socket->flush();
    m_server->close();
    m_isProtocolValid = false;
    // Sometimes Glaxnimate needs another greeting
    QTimer::singleShot(100, this, [ = ] () {
        *m_stream << QString("hello");
        m_socket->flush();
    });
}

int GlaxnimateIpcServer::toMltFps(float frame) const
{
    if (parent->m_producer.get_double("meta.media.frame_rate") > 0) {
        return qRound(frame / parent->m_producer.get_double("meta.media.frame_rate") * MLT.profile().fps());
    }
    return frame;
}

void GlaxnimateIpcServer::onReadyRead()
{
    if (!m_isProtocolValid) {
        QString message;
        *m_stream >> message;
        LOG_DEBUG() << message;
        if (message.startsWith("version ") && message != "version 1") {
            *m_stream << QString("bye");
            m_socket->flush();
            m_server->close();
        } else {
            m_isProtocolValid = true;
        }
    } else if (parent) {
        qreal time = -1.0;
        for (int i = 0; i < 1000 && !m_stream->atEnd(); i++) {
            *m_stream >> time;
        }

        // Only if the frame number is different
        int frameNum = parent->m_producer.get_int(kPlaylistStartProperty) + toMltFps(
                           time) - parent->m_producer.get_int("first_frame");
        if (frameNum != parent->m_frameNum) {
            LOG_DEBUG() << "glaxnimate time =" << time << "=> Shotcut frameNum =" << frameNum;

            if (!parent || !parent->m_glaxnimateProducer
                    || !parent->m_glaxnimateProducer->is_valid()
                    || time < 0.0) {
                MLT.seek(frameNum);
                return;
            }

            // Get the image from MLT
            parent->m_glaxnimateProducer->seek(frameNum);
            std::unique_ptr<Mlt::Frame> frame(parent->m_glaxnimateProducer->get_frame());
            // Use preview scaling
#ifdef Q_OS_MAC
            int scale = Settings.playerPreviewScale() ? Settings.playerPreviewScale() : 720;
#else
            int scale = Settings.playerPreviewScale() ? Settings.playerPreviewScale() : 2160;
#endif
            auto height = qMin(scale, MLT.profile().height());
            auto width = (height == MLT.profile().height()) ? MLT.profile().width() :
                         Util::coerceMultiple(height * MLT.profile().display_aspect_num() /
                                              MLT.profile().display_aspect_den()
                                              * MLT.profile().sample_aspect_den()  / MLT.profile().sample_aspect_num());
            frame->set("consumer.deinterlacer", Settings.playerDeinterlacer().toLatin1().constData());
            frame->set("consumer.top_field_first", -1);
            mlt_image_format format = mlt_image_rgb;
            const uchar *image = frame->get_image(format, width, height);
            if (image) {
                QImage temp(width, height, QImage::Format_RGB888);
                for (int i = 0; i < height; i++) {
                    ::memcpy(temp.scanLine(i), &image[i * 3 * width], temp.bytesPerLine());
                }
                if (MLT.profile().sar() - 1.0 > 0.0001) {
                    // Use QImage to convert to square pixels
                    width = qRound(width * MLT.profile().sar());
                    temp = temp.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                }
                if (copyToShared(temp)) {
                    parent->m_frameNum = frameNum;
                }
            }
        }
    }
}

void GlaxnimateIpcServer::onSocketError(QLocalSocket::LocalSocketError socketError)
{
    switch (socketError) {
    case QLocalSocket::PeerClosedError:
        LOG_DEBUG() << "Glaxnimate closed the connection";
        m_stream.reset();
        m_sharedMemory.reset();
        break;
    default:
        LOG_INFO() << "Glaxnimate IPC error:" << m_socket->errorString();
    }
}

void GlaxnimateIpcServer::onFrameDisplayed(const SharedFrame &frame)
{
    auto image  = frame.get_image(mlt_image_rgb);
    if (image) {
        auto width = frame.get_image_width();
        auto height = frame.get_image_height();
        QImage temp(width, height, QImage::Format_RGB888);
        for (int i = 0; i < height; i++) {
            ::memcpy(temp.scanLine(i), &image[i * 3 * width], temp.bytesPerLine());
        }
        if (copyToShared(temp) && parent) {
            parent->m_frameNum = frame.get_position();
        }
    }
}

GlaxnimateIpcServer &GlaxnimateIpcServer::instance()
{
    static GlaxnimateIpcServer instance;
    return instance;
}

void GlaxnimateIpcServer::newFile(const QString &filename, int duration)
{
    QFile rawr(QStringLiteral(":/resources/glaxnimate.rawr"));
    rawr.open(QIODevice::ReadOnly);
    auto data = rawr.readAll();
    auto json = QJsonDocument::fromJson(data).object();
    rawr.close();

    QJsonValue jsonValue(json);
    modifyJsonValue(jsonValue, "animation.name", QFileInfo(filename).completeBaseName());
    modifyJsonValue(jsonValue, "animation.width", qRound(MLT.profile().width() * MLT.profile().sar()));
    modifyJsonValue(jsonValue, "animation.height", MLT.profile().height());
    modifyJsonValue(jsonValue, "animation.fps", MLT.profile().fps());
    modifyJsonValue(jsonValue, "animation.animation.last_frame", duration - 1);
    modifyJsonValue(jsonValue, "animation.shapes[0].animation.last_frame", duration - 1);
    json = jsonValue.toObject();

    rawr.setFileName(filename);
    rawr.open(QIODevice::WriteOnly);
    rawr.write(QJsonDocument(json).toJson());
    rawr.close();
}

void GlaxnimateIpcServer::reset()
{
    if (m_stream && m_socket && m_stream && QLocalSocket::ConnectedState == m_socket->state()) {
        *m_stream << QString("clear");
        m_socket->flush();
    }
    parent.reset();
}

void GlaxnimateIpcServer::launch(const Mlt::Producer &producer, QString filename,
                                 bool hideCurrentTrack)
{
    parent.reset(new ParentResources);

    if (Settings.playerGPU()) {
        parent->m_producer = producer;
        connect(qobject_cast<Mlt::VideoWidget *>(MLT.videoWidget()), &Mlt::VideoWidget::frameDisplayed,
                this, &GlaxnimateIpcServer::onFrameDisplayed);
    } else {
        LongUiTask longTask(QObject::tr("Edit With Glaxnimate"));
        auto future = QtConcurrent::run([this, &producer, &hideCurrentTrack]() {
            parent->setProducer(producer, hideCurrentTrack);
            return true;
        });
        longTask.wait<bool>(tr("Preparing Glaxnimate preview...."), future);
    }

    if (filename.isEmpty()) {
        filename = QString::fromUtf8(parent->m_producer.get("resource"));
    }

    if (m_server && m_socket && m_stream && QLocalSocket::ConnectedState == m_socket->state()) {
        auto s = QString("open ").append(filename);
        LOG_DEBUG() << s;
        *m_stream << s;
        m_socket->flush();
        parent->m_frameNum = -1;
        return;
    }

    m_server.reset(new QLocalServer);
    connect(m_server.get(), &QLocalServer::newConnection, this, &GlaxnimateIpcServer::onConnect);
    QString name = "shotcut-%1";
    name = name.arg(QCoreApplication::applicationPid());
    QStringList args = {"--ipc", name, filename};
    QProcess childProcess;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("LC_ALL");
    childProcess.setProcessEnvironment(env);
    childProcess.setProgram(Settings.glaxnimatePath());
    if (!m_server->listen(name)) {
        LOG_ERROR() << "failed to start the IPC server:" << m_server->errorString();
        m_server.reset();
        args.clear();
        args << filename;
        // Run without --ipc
    } else  {
        childProcess.setArguments(args);
        if (childProcess.startDetached()) {
            LOG_DEBUG() << Settings.glaxnimatePath() << args.join(' ');
            m_sharedMemory.reset(new QSharedMemory(name));
            return;
        } else {
            // This glaxnimate executable may not support --ipc
            //XXX startDetached is not failing in this case, need something better
            m_server.reset();
            args.clear();
            args << filename;
            // Try without --ipc
        }
    }
    LOG_DEBUG() << Settings.glaxnimatePath() << args.join(' ');
    childProcess.setArguments(args);
    if (!childProcess.startDetached()) {
        LOG_DEBUG() << "failed to launch Glaxnimate with" << Settings.glaxnimatePath();
        QMessageBox dialog(QMessageBox::Information,
                           qApp->applicationName(),
                           tr("The Glaxnimate program was not found.\n\n"
                              "Click OK to open a file dialog to choose its location.\n"
                              "Click Cancel if you do not have Glaxnimate."),
                           QMessageBox::Ok | QMessageBox::Cancel,
                           MAIN.window());
        dialog.setDefaultButton(QMessageBox::Ok);
        dialog.setEscapeButton(QMessageBox::Cancel);
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QMessageBox::Ok) {
            auto path = QFileDialog::getOpenFileName(MAIN.window(), tr("Find Glaxnimate"), QString(), QString(),
                                                     nullptr, Util::getFileDialogOptions());
            if (!path.isEmpty()) {
                args.clear();
                args << "--ipc" << name << filename;
                childProcess.setProgram(path);
                childProcess.setArguments(args);
                if (childProcess.startDetached()) {
                    LOG_DEBUG() << Settings.glaxnimatePath() << args.join(' ');
                    Settings.setGlaxnimatePath(path);
                    LOG_INFO() << "changed Glaxnimate path to" << path;
                    m_sharedMemory.reset(new QSharedMemory(name));
                    return;
                } else {
                    // This glaxnimate executable may not support --ipc
                    m_server.reset();
                    args.clear();
                    args << filename;
                    // Try without --ipc
                    childProcess.setArguments(args);
                    if (childProcess.startDetached()) {
                        LOG_DEBUG() << Settings.glaxnimatePath() << args.join(' ');
                    } else {
                        LOG_WARNING() << "failed to launch Glaxnimate with" << path;
                    }
                }
            }
        }
    }
}

bool GlaxnimateIpcServer::copyToShared(const QImage &image)
{
    if (!m_sharedMemory) {
        return false;
    }
    qint32 sizeInBytes = image.sizeInBytes() + 4 * sizeof(qint32);
    if (sizeInBytes > m_sharedMemory->size()) {
        if (m_sharedMemory->isAttached()) {
            m_sharedMemory->lock();
            m_sharedMemory->detach();
            m_sharedMemory->unlock();
        }
        // over-allocate to avoid recreating
        if (!m_sharedMemory->create(sizeInBytes)) {
            LOG_WARNING() << m_sharedMemory->errorString();
            return false;
        }
    }
    if (m_sharedMemory->isAttached()) {
        m_sharedMemory->lock();

        uchar *to = (uchar *) m_sharedMemory->data();
        // Write the width of the image and move the pointer forward
        qint32 width = image.width();
        ::memcpy(to, &width, sizeof(width));
        to += sizeof(width);

        // Write the height of the image and move the pointer forward
        qint32 height = image.height();
        ::memcpy(to, &height, sizeof(height));
        to += sizeof(height);

        // Write the image format of the image and move the pointer forward
        qint32 imageFormat = image.format();
        ::memcpy(to, &imageFormat, sizeof(imageFormat));
        to += sizeof(imageFormat);

        // Write the bytes per line of the image and move the pointer forward
        qint32 bytesPerLine = image.bytesPerLine();
        ::memcpy(to, &bytesPerLine, sizeof(bytesPerLine));
        to += sizeof(bytesPerLine);

        // Write the raw data of the image and move the pointer forward
        ::memcpy(to, image.constBits(), image.sizeInBytes());

        m_sharedMemory->unlock();
        if (m_stream && m_socket) {
            *m_stream << QString("redraw");
            m_socket->flush();
        }
        return true;
    }
    return false;
}
