/*
 * Copyright (c) 2022-2025 Meltytech, LLC
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

#ifndef GLAXNIMATEPRODUCERWIDGET_H
#define GLAXNIMATEPRODUCERWIDGET_H

#include "abstractproducerwidget.h"
#include "sharedframe.h"

#include <QDataStream>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <QSharedMemory>
#include <QWidget>

class GlaxnimateIpcServer : public QObject
{
    Q_OBJECT

    class ParentResources
    {
    public:
        Mlt::Producer m_producer;
        std::unique_ptr<Mlt::Profile> m_profile;
        std::unique_ptr<Mlt::Producer> m_glaxnimateProducer;
        int m_frameNum = -1;

        void setProducer(const Mlt::Producer &producer, bool hideCurrentTrack);
    };

public:
    std::unique_ptr<ParentResources> parent;
    std::unique_ptr<QLocalServer> m_server;
    std::unique_ptr<QDataStream> m_stream;
    bool m_isProtocolValid = false;
    std::unique_ptr<QSharedMemory> m_sharedMemory;
    QPointer<QLocalSocket> m_socket;

    static GlaxnimateIpcServer &instance();
    static void newFile(const QString &filename, int duration);
    void reset();
    void launch(const Mlt::Producer &producer,
                QString filename = QString(),
                bool hideCurrentTrack = true);

private slots:
    void onConnect();
    void onReadyRead();
    void onSocketError(QLocalSocket::LocalSocketError socketError);
    void onFrameDisplayed(const SharedFrame &frame);

private:
    int toMltFps(float frame) const;
    bool copyToShared(const QImage &image);
    SharedFrame m_sharedFrame;
};

namespace Ui {
class GlaxnimateProducerWidget;
}
class QFileSystemWatcher;
class QLocalServer;
class QDataStream;
class QSharedMemory;

class GlaxnimateProducerWidget : public QWidget, public AbstractProducerWidget
{
    friend GlaxnimateIpcServer;

    Q_OBJECT

public:
    explicit GlaxnimateProducerWidget(QWidget *parent = 0);
    ~GlaxnimateProducerWidget();

    // AbstractProducerWidget overrides
    Mlt::Producer *newProducer(Mlt::Profile &);
    virtual void setProducer(Mlt::Producer *);
    Mlt::Properties getPreset() const;
    void loadPreset(Mlt::Properties &);
    void setLaunchOnNew(bool launch);

signals:
    void producerChanged(Mlt::Producer *);
    void modified();

public slots:
    void rename();

private slots:
    void on_colorButton_clicked();
    void on_preset_selected(void *p);
    void on_preset_saveClicked();
    void on_lineEdit_editingFinished();
    void on_notesTextEdit_textChanged();
    void on_editButton_clicked();
    void onFileChanged(const QString &path);
    void on_reloadButton_clicked();
    void on_durationSpinBox_editingFinished();

private:
    Ui::GlaxnimateProducerWidget *ui;
    QString m_title;
    std::unique_ptr<QFileSystemWatcher> m_watcher;
    bool m_launchOnNew = true;
};

#endif // GLAXNIMATEPRODUCERWIDGET_H
