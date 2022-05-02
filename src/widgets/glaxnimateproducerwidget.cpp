/*
 * Copyright (c) 2022 Meltytech, LLC
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
#include "settings.h"
#include "ui_glaxnimateproducerwidget.h"
#include "shotcut_mlt_properties.h"
#include "util.h"
#include "mltcontroller.h"
#include "qmltypes/qmlapplication.h"
#include <Logger.h>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileSystemWatcher>
#include <QMessageBox>

static const QString kTransparent = QObject::tr("transparent", "Open Other > Animation");

static QString colorToString(const QColor& color)
{
    return (color == QColor(0, 0, 0, 0)) ? kTransparent
           : QString::asprintf("#%02X%02X%02X%02X",
                               qAlpha(color.rgba()),
                               qRed(color.rgba()),
                               qGreen(color.rgba()),
                               qBlue(color.rgba()));
}

static QString colorStringToResource(const QString& s)
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

static void modifyJsonValue(QJsonValue& destValue, const QString& path, const QJsonValue& newValue)
{
    const int indexOfDot = path.indexOf('.');
    const QString dotPropertyName = path.left(indexOfDot);
    const QString dotSubPath = indexOfDot > 0 ? path.mid(indexOfDot + 1) : QString();

    const int indexOfSquareBracketOpen = path.indexOf('[');
    const int indexOfSquareBracketClose = path.indexOf(']');

    const int arrayIndex = path.midRef(indexOfSquareBracketOpen + 1, indexOfSquareBracketClose - indexOfSquareBracketOpen - 1).toInt();

    const QString squareBracketPropertyName = path.left(indexOfSquareBracketOpen);
    const QString squareBracketSubPath = indexOfSquareBracketClose > 0 ? (path.mid(indexOfSquareBracketClose + 1)[0] == '.' ? path.mid(indexOfSquareBracketClose + 2) : path.mid(indexOfSquareBracketClose + 1)) : QString();

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
            useDot = true; // acutally, id doesn't matter, both dot and square bracket don't exist
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
            modifyJsonValue(arrEntry,usedSubPath,newValue);
            arr[arrayIndex] = arrEntry;
            subValue = arr;
        } else if (subValue.isObject()) {
            modifyJsonValue(subValue,usedSubPath,newValue);
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


Mlt::Producer* GlaxnimateProducerWidget::newProducer(Mlt::Profile& profile)
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

    QFile rawr(QStringLiteral(":/resources/glaxnimate.rawr"));
    rawr.open(QIODevice::ReadOnly);
    auto data = rawr.readAll();
    auto json = QJsonDocument::fromJson(data).object();
    rawr.close();

    QJsonValue jsonValue(json);
    auto duration = ui->durationSpinBox->value();
    modifyJsonValue(jsonValue, "animation.name", info.completeBaseName());
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

    Mlt::Producer* p = new Mlt::Producer(profile,
                                         QString("glaxnimate:").append(rawr.fileName()).toUtf8().constData());
    p->set("background", colorStringToResource(ui->colorLabel->text()).toLatin1().constData());

    m_title = info.fileName();
    p->set(kShotcutCaptionProperty, m_title.toUtf8().constData());
    p->set(kShotcutDetailProperty, filename.toUtf8().constData());

    m_watcher.reset(new QFileSystemWatcher({filename}));
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this, &GlaxnimateProducerWidget::onFileChanged);
    launchGlaxnimate(filename);

    return p;
}

void GlaxnimateProducerWidget::setProducer(Mlt::Producer* p)
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
    connect(m_watcher.get(), &QFileSystemWatcher::fileChanged, this, &GlaxnimateProducerWidget::onFileChanged);
}

Mlt::Properties GlaxnimateProducerWidget::getPreset() const
{
    Mlt::Properties p;
    QString color = colorStringToResource(ui->colorLabel->text());
    p.set("background", color.toLatin1().constData());
    return p;
}

void GlaxnimateProducerWidget::loadPreset(Mlt::Properties& p)
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

void GlaxnimateProducerWidget::on_preset_selected(void* p)
{
    Mlt::Properties* properties = (Mlt::Properties*) p;
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
        launchGlaxnimate(QString::fromUtf8(m_producer->get("resource")));
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
    }
}

void GlaxnimateProducerWidget::on_reloadButton_clicked()
{
    if (m_producer && m_producer->is_valid()) {
        m_producer->set("refresh", 1);
        MLT.refreshConsumer();
    }
}

void GlaxnimateProducerWidget::on_durationSpinBox_editingFinished()
{
    if (m_producer && m_producer->is_valid()) {
        m_producer->set("length", m_producer->frames_to_time(ui->durationSpinBox->value(), mlt_time_clock));
        emit producerChanged(m_producer.get());
    }
}

void GlaxnimateProducerWidget::launchGlaxnimate(const QString &filename)
{
    if (!QProcess::startDetached(Settings.glaxnimatePath(), {filename})) {
        LOG_DEBUG() << "failed to launch Glaxnimate with" << Settings.glaxnimatePath();
        QMessageBox dialog(QMessageBox::Information,
                           qApp->applicationName(),
                           tr("The Glaxnimate program was not found.\n\n"
                              "Click OK to open a file dialog to choose its location.\n"
                              "Click Cancel if you do not have Glaxnimate."),
                           QMessageBox::Ok | QMessageBox::Cancel,
                           this);
        dialog.setDefaultButton(QMessageBox::Ok);
        dialog.setEscapeButton(QMessageBox::Cancel);
        dialog.setWindowModality(QmlApplication::dialogModality());
        if (dialog.exec() == QMessageBox::Ok) {
            auto path = QFileDialog::getOpenFileName(this, tr("Find Glaxnimate"), QString(), QString(),
                        nullptr, Util::getFileDialogOptions());
            if (!path.isEmpty()) {
                if (QProcess::startDetached(path, {filename})) {
                    Settings.setGlaxnimatePath(path);
                    LOG_INFO() << "changed Glaxnimate path to" << path;
                } else {
                    LOG_WARNING() << "failed to launch Glaxnimate with" << path;
                }
            }
        }
    }
}
