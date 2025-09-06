/*
 * Copyright (c) 2012-2025 Meltytech, LLC
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

#include "imageproducerwidget.h"
#include "ui_imageproducerwidget.h"

#include "Logger.h"
#include "dialogs/filedatedialog.h"
#include "dialogs/listselectiondialog.h"
#include "mainwindow.h"
#include "proxymanager.h"
#include "qmltypes/qmlapplication.h"
#include "settings.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include <unistd.h>

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>

// This legacy property is only used in this widget.
#define kShotcutResourceProperty "shotcut_resource"
static const auto kImageMediaType = QLatin1String("image");

static QString GetFilenameFromProducer(Mlt::Producer *producer, bool useOriginal = true);

ImageProducerWidget::ImageProducerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ImageProducerWidget)
    , m_defaultDuration(-1)
{
    ui->setupUi(this);
    Util::setColorsToHighlight(ui->filenameLabel, QPalette::Base);
}

ImageProducerWidget::~ImageProducerWidget()
{
    delete ui;
}

Mlt::Producer *ImageProducerWidget::newProducer(Mlt::Profile &profile)
{
    QString resource = QString::fromUtf8(m_producer->get("resource"));
    if (!resource.contains("?begin=") && m_producer->get("begin")) {
        resource.append(QStringLiteral("?begin=%1").arg(m_producer->get("begin")));
    }
    LOG_DEBUG() << resource;
    Mlt::Producer *p = new Mlt::Producer(profile, resource.toUtf8().constData());
    if (p->is_valid()) {
        if (ui->durationSpinBox->value() > p->get_length())
            p->set("length", p->frames_to_time(ui->durationSpinBox->value(), mlt_time_clock));
        p->set_in_and_out(0, ui->durationSpinBox->value() - 1);
    }
    return p;
}

void ImageProducerWidget::setProducer(Mlt::Producer *p)
{
    AbstractProducerWidget::setProducer(p);
    if (m_defaultDuration == -1)
        m_defaultDuration = m_producer->get_length();
    QString resource;
    if (m_producer->get(kShotcutResourceProperty)) {
        resource = QString::fromUtf8(m_producer->get(kShotcutResourceProperty));
    } else if (m_producer->get(kOriginalResourceProperty)) {
        resource = QString::fromUtf8(m_producer->get(kOriginalResourceProperty));
    } else {
        resource = QString::fromUtf8(m_producer->get("resource"));
        p->set("ttl", 1);
    }
    QString name = Util::baseName(resource);
    QString caption = m_producer->get(kShotcutCaptionProperty);
    if (caption.isEmpty()) {
        caption = name;
        m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
    }
    ui->filenameLabel->setText(
        ui->filenameLabel->fontMetrics().elidedText(caption, Qt::ElideLeft, width() - 30));
    updateDuration();
    resource = QDir::toNativeSeparators(resource);
    ui->filenameLabel->setToolTip(resource);
    bool isProxy = m_producer->get_int(kIsProxyProperty)
                   && m_producer->get(kOriginalResourceProperty);
    ui->resolutionLabel->setText(QStringLiteral("%1x%2 %3")
                                     .arg(p->get("meta.media.width"))
                                     .arg(p->get("meta.media.height"))
                                     .arg(isProxy ? tr("(PROXY)") : ""));
    ui->aspectNumSpinBox->blockSignals(true);
    if (p->get(kAspectRatioNumerator) && p->get(kAspectRatioDenominator)) {
        ui->aspectNumSpinBox->setValue(p->get_int(kAspectRatioNumerator));
        ui->aspectDenSpinBox->setValue(p->get_int(kAspectRatioDenominator));
    } else {
        double sar = m_producer->get_double("aspect_ratio");
        if (m_producer->get("force_aspect_ratio"))
            sar = m_producer->get_double("force_aspect_ratio");
        if (sar == 1.0) {
            ui->aspectNumSpinBox->setValue(1);
            ui->aspectDenSpinBox->setValue(1);
        } else {
            ui->aspectNumSpinBox->setValue(1000 * sar);
            ui->aspectDenSpinBox->setValue(1000);
        }
    }
    ui->aspectNumSpinBox->blockSignals(false);
    if (m_producer->get_int("ttl"))
        ui->repeatSpinBox->setValue(m_producer->get_int("ttl"));
    ui->sequenceCheckBox->setChecked(m_producer->get_int(kShotcutSequenceProperty));
    ui->repeatSpinBox->setEnabled(m_producer->get_int(kShotcutSequenceProperty));
    ui->durationSpinBox->setEnabled(!m_producer->get_int(kShotcutSequenceProperty));
    ui->defaultDurationButton->setEnabled(ui->durationSpinBox->isEnabled());
    ui->notesTextEdit->setPlainText(QString::fromUtf8(m_producer->get(kCommentProperty)));
}

void ImageProducerWidget::updateDuration()
{
    if (m_producer->get(kFilterOutProperty))
        ui->durationSpinBox->setValue(m_producer->get_int(kFilterOutProperty)
                                      - m_producer->get_int(kFilterInProperty) + 1);
    else
        ui->durationSpinBox->setValue(m_producer->get_playtime());
}

void ImageProducerWidget::rename()
{
    ui->filenameLabel->setFocus();
    ui->filenameLabel->selectAll();
}

void ImageProducerWidget::reopen(Mlt::Producer *p)
{
    int position = m_producer->position();
    if (position > p->get_out())
        position = p->get_out();
    p->set("in", m_producer->get_in());
    MLT.stop();
    if (MLT.setProducer(p)) {
        AbstractProducerWidget::setProducer(nullptr);
        return;
    }
    setProducer(p);
    emit producerReopened(false);
    emit producerChanged(p);
    if (p->get_int(kShotcutSequenceProperty)) {
        MLT.play();
    } else {
        MLT.seek(position);
    }
}

void ImageProducerWidget::recreateProducer()
{
    QString resource = m_producer->get("resource");
    if (!resource.startsWith("qimage:") && !resource.startsWith("pixbuf:")) {
        QString serviceName = m_producer->get("mlt_service");
        if (!serviceName.isEmpty()) {
            if (QFileInfo(resource).isRelative()) {
                QString basePath = QFileInfo(MAIN.fileName()).canonicalPath();
                QFileInfo fi(basePath, resource);
                resource = fi.filePath();
            }
            resource.prepend(':').prepend(serviceName);
            m_producer->set("resource", resource.toUtf8().constData());
        }
    }
    Mlt::Producer *p = newProducer(MLT.profile());
    if (!p || !p->is_valid()) {
        // retry
        ::sleep(1);
        p = newProducer(MLT.profile());
    }
    if (!p || !p->is_valid()) {
        LOG_ERROR() << "failed to recreate producer for:" + resource;
        return;
    }
    p->pass_list(*m_producer,
                 "force_aspect_ratio," kAspectRatioNumerator "," kAspectRatioDenominator
                 ", begin, ttl," kShotcutResourceProperty
                 ", autolength, length," kShotcutSequenceProperty ", " kPlaylistIndexProperty
                 ", " kCommentProperty "," kOriginalResourceProperty "," kDisableProxyProperty
                 "," kIsProxyProperty);
    Mlt::Controller::copyFilters(*m_producer, *p);
    if (m_producer->get(kMultitrackItemProperty)) {
        emit producerChanged(p);
        delete p;
    } else {
        reopen(p);
    }
    if (m_watcher) {
        m_watcher.reset(new QFileSystemWatcher({GetFilenameFromProducer(producer())}));
        connect(m_watcher.get(),
                &QFileSystemWatcher::fileChanged,
                this,
                &ImageProducerWidget::recreateProducer);
    }
}

void ImageProducerWidget::on_reloadButton_clicked()
{
    if (!m_producer)
        return;
    recreateProducer();
}

void ImageProducerWidget::on_aspectNumSpinBox_valueChanged(int)
{
    if (m_producer) {
        double new_sar = double(ui->aspectNumSpinBox->value())
                         / double(ui->aspectDenSpinBox->value());
        double sar = m_producer->get_double("aspect_ratio");
        if (m_producer->get("force_aspect_ratio") || new_sar != sar) {
            m_producer->set("force_aspect_ratio", QString::number(new_sar).toLatin1().constData());
            m_producer->set(kAspectRatioNumerator,
                            ui->aspectNumSpinBox->text().toLatin1().constData());
            m_producer->set(kAspectRatioDenominator,
                            ui->aspectDenSpinBox->text().toLatin1().constData());
        }
        emit producerChanged(producer());
    }
}

void ImageProducerWidget::on_aspectDenSpinBox_valueChanged(int i)
{
    on_aspectNumSpinBox_valueChanged(i);
}

void ImageProducerWidget::on_durationSpinBox_editingFinished()
{
    if (!m_producer)
        return;
    if (ui->durationSpinBox->value() == m_producer->get_playtime())
        return;
    recreateProducer();
}

void ImageProducerWidget::on_sequenceCheckBox_clicked(bool checked)
{
    QString resource = m_producer->get("resource");
    if (checked && m_producer->get_int(kIsProxyProperty)
        && m_producer->get(kOriginalResourceProperty)) {
        // proxy is not currently supported for image sequence, disable it
        resource = m_producer->get(kOriginalResourceProperty);
        m_producer->set(kDisableProxyProperty, 1);
        m_producer->Mlt::Properties::clear(kIsProxyProperty);
        m_producer->Mlt::Properties::clear(kOriginalResourceProperty);
    }
    ui->repeatSpinBox->setEnabled(checked);
    if (checked && !m_producer->get(kShotcutResourceProperty))
        m_producer->set(kShotcutResourceProperty, resource.toUtf8().constData());
    m_producer->set(kShotcutSequenceProperty, checked);
    m_producer->set("autolength", checked);
    m_producer->set("ttl", ui->repeatSpinBox->value());
    if (checked) {
        QFileInfo info(resource);
        QString name(info.fileName());
        QString begin = "";
        int i = name.length();
        int count = 0;

        // find the last numeric digit
        for (; i && !name[i - 1].isDigit(); i--) {
        };
        // count the digits and build the begin value
        for (; i && name[i - 1].isDigit(); i--, count++)
            begin.prepend(name[i - 1]);
        if (count) {
            m_producer->set("begin", begin.toLatin1().constData());
            name.replace(i, count, QStringLiteral("0%1d").arg(count).prepend('%'));
            QString serviceName = m_producer->get("mlt_service");
            if (!serviceName.isEmpty())
                resource = serviceName + ":" + info.path() + "/" + name;
            else
                resource = info.path() + "/" + name;
            m_producer->set("resource", resource.toUtf8().constData());

            // Count the number of consecutive files.
            // Allow for gaps of up to 100 missing files as is supported by producer_qimage
            MAIN.showStatusMessage(tr("Getting length of image sequence..."));
            QCoreApplication::processEvents();
            name = info.fileName();
            name.replace(i, count, "%1");
            resource = info.path().append('/').append(name);
            int imageCount = 0;
            i = begin.toInt();
            for (int gap = 0; gap < 100;) {
                if (QFile::exists(resource.arg(i, count, 10, QChar('0')))) {
                    imageCount++;
                    gap = 0;
                } else {
                    gap++;
                }
                i++;
                if (i % 100 == 0)
                    QCoreApplication::processEvents();
            }
            m_producer->set("length",
                            m_producer->frames_to_time(imageCount * m_producer->get_int("ttl"),
                                                       mlt_time_clock));
            ui->durationSpinBox->setValue(imageCount);
            ui->durationSpinBox->setEnabled(false);
            MAIN.showStatusMessage(tr("Reloading image sequence..."));
            QCoreApplication::processEvents();
        }
    } else {
        m_producer->Mlt::Properties::clear(kDisableProxyProperty);
        m_producer->Mlt::Properties::clear("begin");
        m_producer->set("resource", m_producer->get(kShotcutResourceProperty));
        m_producer->set("length",
                        m_producer->frames_to_time(qRound(MLT.profile().fps()
                                                          * Mlt::kMaxImageDurationSecs),
                                                   mlt_time_clock));
        ui->durationSpinBox->setValue(qRound(MLT.profile().fps() * Settings.imageDuration()));
        ui->durationSpinBox->setEnabled(true);
    }
    ui->defaultDurationButton->setEnabled(ui->durationSpinBox->isEnabled());
    recreateProducer();
}

void ImageProducerWidget::on_repeatSpinBox_editingFinished()
{
    m_producer->set("ttl", ui->repeatSpinBox->value());
    ui->durationSpinBox->setValue(m_producer->get_length());
    MAIN.showStatusMessage(tr("Reloading image sequence..."));
    QCoreApplication::processEvents();
    recreateProducer();
}

void ImageProducerWidget::on_defaultDurationButton_clicked()
{
    Settings.setImageDuration(ui->durationSpinBox->value() / MLT.profile().fps());
}

void ImageProducerWidget::on_notesTextEdit_textChanged()
{
    QString existing = QString::fromUtf8(m_producer->get(kCommentProperty));
    if (ui->notesTextEdit->toPlainText() != existing) {
        m_producer->set(kCommentProperty, ui->notesTextEdit->toPlainText().toUtf8().constData());
        emit modified();
    }
}

void ImageProducerWidget::on_menuButton_clicked()
{
    QMenu menu;
    if (!MLT.resource().contains("://")) // not a network stream
        menu.addAction(ui->actionShowInFiles);
    if (!MLT.resource().contains("://")) // not a network stream
        menu.addAction(ui->actionOpenFolder);
    menu.addAction(ui->actionCopyFullFilePath);
    menu.addAction(ui->actionSetFileDate);
    menu.addAction(ui->actionReset);
    menu.exec(ui->menuButton->mapToGlobal(QPoint(0, 0)));
}

static QString GetFilenameFromProducer(Mlt::Producer *producer, bool useOriginal)
{
    QString resource;
    if (useOriginal && producer->get(kOriginalResourceProperty)) {
        resource = QString::fromUtf8(producer->get(kOriginalResourceProperty));
    } else if (producer->get(kShotcutResourceProperty)) {
        resource = QString::fromUtf8(producer->get(kShotcutResourceProperty));
    } else {
        resource = QString::fromUtf8(producer->get("resource"));
    }
    if (QFileInfo(resource).isRelative()) {
        QString basePath = QFileInfo(MAIN.fileName()).canonicalPath();
        QFileInfo fi(basePath, resource);
        resource = fi.filePath();
    }
    return resource;
}

void ImageProducerWidget::on_actionCopyFullFilePath_triggered()
{
    auto s = GetFilenameFromProducer(producer());
    qApp->clipboard()->setText(QDir::toNativeSeparators(s));
}

void ImageProducerWidget::on_actionOpenFolder_triggered()
{
    Util::showInFolder(GetFilenameFromProducer(producer()));
}

void ImageProducerWidget::on_actionSetFileDate_triggered()
{
    QString resource = GetFilenameFromProducer(producer());
    FileDateDialog dialog(resource, producer(), this);
    dialog.setModal(QmlApplication::dialogModality());
    dialog.exec();
}

void ImageProducerWidget::on_filenameLabel_editingFinished()
{
    if (m_producer) {
        auto caption = ui->filenameLabel->text();
        if (caption.isEmpty()) {
            caption = Util::baseName(GetFilenameFromProducer(m_producer.data()));
            ui->filenameLabel->setText(caption);
            m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
        } else {
            m_producer->set(kShotcutCaptionProperty, caption.toUtf8().constData());
        }
        emit modified();
    }
}

void ImageProducerWidget::on_actionDisableProxy_triggered(bool checked)
{
    if (checked) {
        producer()->set(kDisableProxyProperty, 1);

        // Replace with original
        if (producer()->get_int(kIsProxyProperty) && producer()->get(kOriginalResourceProperty)) {
            Mlt::Producer original(MLT.profile(), producer()->get(kOriginalResourceProperty));
            if (original.is_valid()) {
                original.set(kDisableProxyProperty, 1);
                MAIN.replaceAllByHash(Util::getHash(original), original, true);
            }
        }
    } else {
        producer()->Mlt::Properties::clear(kDisableProxyProperty);
    }
}

void ImageProducerWidget::on_actionMakeProxy_triggered()
{
    ProxyManager::generateImageProxy(*producer());
}

void ImageProducerWidget::on_actionDeleteProxy_triggered()
{
    // Delete the file if it exists
    QString hash = Util::getHash(*producer());
    QString fileName = hash + ProxyManager::imageFilenameExtension();
    QDir dir = ProxyManager::dir();
    LOG_DEBUG() << "removing" << dir.filePath(fileName);
    dir.remove(dir.filePath(fileName));

    // Delete the pending file if it exists));
    fileName = hash + ProxyManager::pendingImageExtension();
    dir.remove(dir.filePath(fileName));

    // Replace with original
    if (producer()->get_int(kIsProxyProperty) && producer()->get(kOriginalResourceProperty)) {
        Mlt::Producer original(MLT.profile(), producer()->get(kOriginalResourceProperty));
        if (original.is_valid()) {
            MAIN.replaceAllByHash(hash, original, true);
        }
    }
}

void ImageProducerWidget::on_actionCopyHashCode_triggered()
{
    qApp->clipboard()->setText(Util::getHash(*producer()));
    QMessageBox::information(this,
                             qApp->applicationName(),
                             tr("The hash code below is already copied to your clipboard:\n\n")
                                 + Util::getHash(*producer()),
                             QMessageBox::Ok);
}

void ImageProducerWidget::on_proxyButton_clicked()
{
    QMenu menu;
    if (ProxyManager::isValidImage(*producer())) {
        menu.addAction(ui->actionMakeProxy);
    }
#ifndef Q_OS_WIN
    menu.addAction(ui->actionDeleteProxy);
#endif
    menu.addAction(ui->actionDisableProxy);
    menu.addAction(ui->actionCopyHashCode);
    bool proxyDisabled = m_producer->get_int(kDisableProxyProperty);
    ui->actionMakeProxy->setDisabled(proxyDisabled);
    ui->actionDisableProxy->setChecked(proxyDisabled);
    menu.exec(ui->proxyButton->mapToGlobal(QPoint(0, 0)));
}

void ImageProducerWidget::on_actionShowInFiles_triggered()
{
    emit showInFiles(GetFilenameFromProducer(producer()));
}

void ImageProducerWidget::on_openWithButton_clicked()
{
    const auto filePath = GetFilenameFromProducer(producer());
    QMenu menu;
    auto action = new QAction(tr("System Default"), this);
    menu.addAction(action);
    connect(action, &QAction::triggered, this, [=]() {
        LOG_DEBUG() << filePath;
#if defined(Q_OS_WIN)
        const auto scheme = QLatin1String("file:///");
#else
        const auto scheme = QLatin1String("file://");
#endif
        if (QDesktopServices::openUrl({scheme + filePath, QUrl::TolerantMode})) {
            m_watcher.reset(new QFileSystemWatcher({filePath}));
            connect(m_watcher.get(),
                    &QFileSystemWatcher::fileChanged,
                    this,
                    &ImageProducerWidget::recreateProducer);
        }
    });

    menu.addSeparator();
    // custom options
    auto programs = Settings.filesOpenOther(kImageMediaType);
    for (const auto &program : programs) {
        auto action = menu.addAction(QFileInfo(program).baseName(), this, [=]() {
            LOG_DEBUG() << program << filePath;
            if (QProcess::startDetached(program, {QDir::toNativeSeparators(filePath)})) {
                m_watcher.reset(new QFileSystemWatcher({filePath}));
                connect(m_watcher.get(),
                        &QFileSystemWatcher::fileChanged,
                        this,
                        &ImageProducerWidget::recreateProducer);
            }
        });
        action->setObjectName(program);
        menu.addAction(action);
    }
    menu.addSeparator();

    action = new QAction(tr("Other..."), this);
    menu.addAction(action);
    connect(action, &QAction::triggered, this, &ImageProducerWidget::onOpenOtherAdd);

    action = new QAction(tr("Remove..."), this);
    menu.addAction(action);
    connect(action, &QAction::triggered, this, &ImageProducerWidget::onOpenOtherRemove);

    menu.exec(ui->openWithButton->mapToGlobal(QPoint(0, 0)));
}

void ImageProducerWidget::onOpenOtherAdd()
{
    LOG_DEBUG();
    const auto filePath = GetFilenameFromProducer(producer());
    if (filePath.isEmpty())
        return;

    QString dir("/usr/bin");
    QString filter;
#if defined(Q_OS_WIN)
    dir = QStringLiteral("C:/Program Files");
    filter = tr("Executable Files (*.exe);;All Files (*)");
#elif defined(Q_OS_MAC)
    dir = QStringLiteral("/Applications");
#endif
    const auto program = QFileDialog::getOpenFileName(MAIN.window(),
                                                      tr("Choose Executable"),
                                                      dir,
                                                      filter,
                                                      nullptr,
                                                      Util::getFileDialogOptions());
    if (!program.isEmpty()) {
        LOG_DEBUG() << program << filePath;
        if (QProcess::startDetached(program, {QDir::toNativeSeparators(filePath)})) {
            Settings.setFilesOpenOther(kImageMediaType, program);
            m_watcher.reset(new QFileSystemWatcher({filePath}));
            connect(m_watcher.get(),
                    &QFileSystemWatcher::fileChanged,
                    this,
                    &ImageProducerWidget::recreateProducer);
        }
    }
}

void ImageProducerWidget::onOpenOtherRemove()
{
    auto ls = Settings.filesOpenOther(kImageMediaType);
    ls.sort(Qt::CaseInsensitive);
    QStringList programs;
    std::for_each(ls.begin(), ls.end(), [&](const QString &s) {
        programs << QDir::toNativeSeparators(s);
    });
    ListSelectionDialog dialog(programs, this);
    dialog.setWindowModality(QmlApplication::dialogModality());
    dialog.setWindowTitle(tr("Remove From Open With"));
    if (QDialog::Accepted == dialog.exec()) {
        for (auto program : dialog.selection()) {
            program = QDir::fromNativeSeparators(program);
            Settings.removeFilesOpenOther(kImageMediaType, program);
        }
    }
}

void ImageProducerWidget::on_actionReset_triggered()
{
    if (!m_producer)
        return;

    const char *s = m_producer->get(kShotcutResourceProperty);
    if (!s)
        s = m_producer->get(kOriginalResourceProperty);
    if (!s)
        s = m_producer->get("resource");
    if (!s)
        return;
    Mlt::Producer *p = new Mlt::Producer(MLT.profile(), s);
    if (!p->is_valid()) {
        LOG_ERROR() << "failed to recreate image producer for:" << s;
        return;
    }
    if (ui->durationSpinBox->value() > p->get_length())
        p->set("length", p->frames_to_time(ui->durationSpinBox->value(), mlt_time_clock));
    p->set_in_and_out(0, ui->durationSpinBox->value() - 1);
    Mlt::Controller::copyFilters(*m_producer, *p);
    if (m_producer->get(kMultitrackItemProperty)) {
        emit producerChanged(p);
        delete p;
    } else {
        reopen(p);
    }
}
