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

#ifndef SHOTCUT_API_APIPROJECT_H
#define SHOTCUT_API_APIPROJECT_H

#include "api/apiobject.h"

#include <QVariantMap>

namespace Shotcut {
namespace Api {

class ApiProject : public ApiObject
{
    Q_OBJECT
    Q_CLASSINFO("doc", "The open project file and its video mode.")
    Q_CLASSINFO("doc.fileName", "Full path of the project file; empty if never saved.")
    Q_CLASSINFO("doc.name", "Project name shown in the title bar.")
    Q_CLASSINFO("doc.modified", "Whether there are unsaved changes.")
    Q_CLASSINFO("doc.projectFolder", "Project folder, if the project uses one.")
    Q_CLASSINFO("doc.profile",
                "Video mode: width, height, fps, frame rate fraction, aspect ratios, progressive, "
                "colorspace, and isAutomatic (the mode follows the first clip added).")
    Q_PROPERTY(QString fileName READ fileName NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(bool modified READ isModified NOTIFY changed)
    Q_PROPERTY(QString projectFolder READ projectFolder NOTIFY changed)
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY changed)

public:
    explicit ApiProject(QObject *parent = nullptr);

    QString fileName() const;
    QString name() const;
    bool isModified() const;
    QString projectFolder() const;
    QVariantMap profile() const;

signals:
    void changed();
};

} // namespace Api
} // namespace Shotcut

#endif // SHOTCUT_API_APIPROJECT_H
