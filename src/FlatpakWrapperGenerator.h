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

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

struct AppEntry
{
    QString appId;
    QString branch; // e.g., "stable"
};

class FlatpakWrapperGenerator
{
public:
    void setOutputDir(const QString &dir) { m_outputDir = dir; }
    void setForce(bool v) { m_force = v; }
    void setDryRun(bool v) { m_dryRun = v; }
    void setPrefix(const QString &p) { m_prefix = p; }

    bool generateAllInstalled();
    bool generateFor(const QStringList &appIds);

private:
    bool ensureOutputDir() const;
    QList<AppEntry> listInstalledApps() const;
    QString getBranchForAppId(const QString &appId) const;
    bool writeWrapper(const AppEntry &app) const;
    QString buildWrapperScript(const AppEntry &app) const;

    QString m_outputDir;
    bool m_force = false;
    bool m_dryRun = false;
    QString m_prefix;
};
