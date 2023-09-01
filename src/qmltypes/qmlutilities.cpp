/*
 * Copyright (c) 2013-2023 Meltytech, LLC
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

#include "qmltypes/qmlapplication.h"
#include "qmltypes/colordialog.h"
#include "qmltypes/colorpickeritem.h"
#include "qmltypes/colorwheelitem.h"
#include "qmltypes/filedialog.h"
#include "qmltypes/fontdialog.h"
#include "qmltypes/messagedialog.h"
#include "qmltypes/qmleditmenu.h"
#include "qmltypes/qmlprofile.h"
#include "qmltypes/qmlutilities.h"
#include "qmltypes/qmlfile.h"
#include "qmltypes/qmlfilter.h"
#include "qmltypes/qmlmarkermenu.h"
#include "qmltypes/qmlmetadata.h"
#include "qmltypes/qmlrichtext.h"
#include "qmltypes/qmlrichtextmenu.h"
#include "qmltypes/timelineitems.h"
#include "settings.h"
#include "models/metadatamodel.h"
#include "models/keyframesmodel.h"
#include <QCoreApplication>
#include <QSysInfo>
#include <QCursor>
#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>

QmlUtilities::QmlUtilities(QObject *parent) :
    QObject(parent)
{
}

void QmlUtilities::registerCommonTypes()
{
    qmlRegisterType<QmlFile>("org.shotcut.qml", 1, 0, "File");
    qmlRegisterType<QmlFilter>("org.shotcut.qml", 1, 0, "Filter");
    qmlRegisterType<QmlMetadata>("org.shotcut.qml", 1, 0, "Metadata");
    qmlRegisterAnonymousType<QmlKeyframesMetadata>("org.shotcut.qml", 1);
    qmlRegisterType<QmlKeyframesParameter>("org.shotcut.qml", 1, 0, "Parameter");
    qmlRegisterType<QmlRichText>("org.shotcut.qml", 1, 0, "RichText");
    qmlRegisterType<KeyframesModel>("org.shotcut.qml", 1, 0, "KeyframesModel");
    qmlRegisterType<QmlUtilities>("org.shotcut.qml", 1, 0, "Utilities");
    // MetadataModel is registered to access its MetadataFilter enum.
    qmlRegisterUncreatableType<MetadataModel>("org.shotcut.qml", 1, 0, "MetadataModel",
                                              "You cannot create a MetadataModel from QML.");
    qmlRegisterUncreatableType<ShotcutSettings>("org.shotcut.qml", 1, 0, "Settings",
                                                "You cannot create a Settings from QML.");
    qmlRegisterType<ColorPickerItem>("Shotcut.Controls", 1, 0, "ColorPickerItem");
    qmlRegisterType<ColorWheelItem>("Shotcut.Controls", 1, 0, "ColorWheelItem");
    qmlRegisterType<QmlMarkerMenu>("Shotcut.Controls", 1, 0, "MarkerMenu");
    qmlRegisterType<QmlEditMenu>("Shotcut.Controls", 1, 0, "EditContextMenu");
    qmlRegisterType<QmlRichTextMenu>("Shotcut.Controls", 1, 0, "RichTextMenu");
    qmlRegisterType<ColorDialog>("Shotcut.Controls", 1, 0, "ColorDialog");
    qmlRegisterType<FontDialog>("Shotcut.Controls", 1, 0, "FontDialog");
    qmlRegisterType<MessageDialog>("Shotcut.Controls", 1, 0, "MessageDialog");
    qmlRegisterType<FileDialog>("Shotcut.Controls", 1, 0, "FileDialog");
    registerTimelineItems();
}

void QmlUtilities::setCommonProperties(QQmlContext *context)
{
    context->setContextProperty("settings", &ShotcutSettings::singleton());
    context->setContextProperty("application", &QmlApplication::singleton());
    context->setContextProperty("profile", &QmlProfile::singleton());
}

QDir QmlUtilities::qmlDir()
{
    QDir dir = QmlApplication::dataDir();
    dir.cd("shotcut");
    dir.cd("qml");
    return dir;
}

QQmlEngine *QmlUtilities::sharedEngine()
{
    static QQmlEngine *s_engine = 0;
    if (!s_engine)
        s_engine = new QQmlEngine;
    return s_engine;
}

QUrl QmlUtilities::blankVui()
{
    QDir dir = qmlDir();
    dir.cd("modules");
    dir.cd("Shotcut");
    dir.cd("Controls");
    return QUrl::fromLocalFile(dir.absoluteFilePath("VuiBase.qml"));
}
