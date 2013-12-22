/*
 * Copyright (c) 2013 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "timelinedock.h"
#include "ui_timelinedock.h"
#include "qmltypes/qmlutilities.h"
#include "models/multitrackmodel.h"
#include "qmltypes/thumbnailprovider.h"

#include <QtQml>
#include <QtQuick>

TimelineDock::TimelineDock(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::TimelineDock),
    m_position(-1)
{
    ui->setupUi(this);
    delete ui->treeView;
    toggleViewAction()->setIcon(windowIcon());

    qmlRegisterType<MultitrackModel>("Shotcut.Models", 1, 0, "MultitrackModel");

    QQuickView* qqview = new QQuickView;
    QDir importPath = QmlUtilities::qmlDir();
    importPath.cd("modules");
    qqview->engine()->addImportPath(importPath.path());
    qqview->engine()->addImageProvider(QString("thumbnail"), new ThumbnailProvider);
    qqview->rootContext()->setContextProperty("timeline", this);
    qqview->rootContext()->setContextProperty("multitrack", &m_model);
    qqview->setResizeMode(QQuickView::SizeRootObjectToView);
    qqview->setColor(palette().window().color());

    importPath = QmlUtilities::qmlDir();
    importPath.cd("timeline");
    qqview->setSource(QUrl::fromLocalFile(importPath.filePath("timeline.qml")));
    QWidget* container = QWidget::createWindowContainer(qqview);
    container->setFocusPolicy(Qt::TabFocus);

    ui->verticalLayout->addWidget(container);
    connect(MLT.videoWidget(), SIGNAL(frameReceived(Mlt::QFrame)), this, SLOT(onShowFrame(Mlt::QFrame)));
}

TimelineDock::~TimelineDock()
{
    delete ui;
}

void TimelineDock::setPosition(int position)
{
    emit seeked(position);
}

QString TimelineDock::timecode(int frames)
{
    return MLT.producer()->frames_to_time(frames, mlt_time_smpte);
}

void TimelineDock::onShowFrame(Mlt::QFrame frame)
{
    if (MLT.isMultitrack()) {
        m_position = frame.frame()->get_position();
        emit positionChanged();
    }
}

void TimelineDock::onSeeked(int position)
{
    if (MLT.isMultitrack()) {
        m_position = position;
        emit positionChanged();
    }
}
