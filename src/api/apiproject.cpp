/*
 * Copyright (c) 2026 Meltytech, LLC
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

#include "api/apiproject.h"

#include "mainwindow.h"
#include "mltcontroller.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QUndoStack>

namespace Shotcut {
namespace Api {

ApiProject::ApiProject(QObject *parent)
    : ApiObject(parent)
{
    connect(&MAIN, &QWidget::windowTitleChanged, this, &ApiProject::changed);
    connect(&MAIN, &MainWindow::profileChanged, this, &ApiProject::changed);
    connect(&MAIN, &MainWindow::producerOpened, this, &ApiProject::changed);
    connect(MAIN.undoStack(), &QUndoStack::cleanChanged, this, &ApiProject::changed);
}

QString ApiProject::fileName() const
{
    return MAIN.fileName();
}

QString ApiProject::name() const
{
    const QString file = fileName();
    return file.isEmpty() ? QCoreApplication::translate("MainWindow", "Untitled")
                          : QFileInfo(file).completeBaseName();
}

bool ApiProject::isModified() const
{
    return MAIN.isWindowModified();
}

QString ApiProject::projectFolder() const
{
    return MLT.projectFolder();
}

QVariantMap ApiProject::profile() const
{
    auto &profile = MLT.profile();
    QVariantMap result;
    result["isAutomatic"] = !profile.is_explicit();
    result["width"] = profile.width();
    result["height"] = profile.height();
    result["fps"] = profile.fps();
    result["frameRateNum"] = profile.frame_rate_num();
    result["frameRateDen"] = profile.frame_rate_den();
    result["sampleAspectNum"] = profile.sample_aspect_num();
    result["sampleAspectDen"] = profile.sample_aspect_den();
    result["displayAspectNum"] = profile.display_aspect_num();
    result["displayAspectDen"] = profile.display_aspect_den();
    result["progressive"] = profile.progressive() != 0;
    result["colorspace"] = profile.colorspace();
    return result;
}

} // namespace Api
} // namespace Shotcut
