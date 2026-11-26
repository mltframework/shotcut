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

#ifndef PROXYENCODE_H
#define PROXYENCODE_H

#include <QPoint>
#include <QString>
#include <QStringList>

// Numeric values match ProxyManager::ScanMode.
constexpr int kProxyScanAutomatic = 0;
constexpr int kProxyScanProgressive = 1;
constexpr int kProxyScanTopFieldFirst = 2;
constexpr int kProxyScanBottomFieldFirst = 3;

// Native is the OS this binary was built for. The other values exist so the
// command can be checked without rebuilding for that OS.
enum class ProxyHwPolicy { Native, Windows, Mac, Generic };

struct ProxyD3d11Surface
{
    int alignedWidth = 0;
    int alignedHeight = 0;
    bool fits = false;
};

struct ProxyVideoRequest
{
    QString resource;
    QString outputFile;
    int resolution = 540;
    bool fullRange = false;
    int scanMode = kProxyScanAutomatic;
    QPoint aspectRatio;
    int colorspace = 0;
    int mediaHeight = 0;
    int videoIndex = 0;
    int audioIndex = 0;
    bool iPhoneAmbisonic = false;
    bool hardware = false;
    QStringList hardwareCodecs;
    QString codecName;
    int pictureWidth = 0;
    int pictureHeight = 0;
    ProxyHwPolicy policy = ProxyHwPolicy::Native;
};

struct ProxyVideoCommand
{
    QStringList args;
    QStringList fallbackArgs;
};

ProxyHwPolicy nativeProxyHwPolicy();
ProxyD3d11Surface proxyD3d11Surface(const QString &codecName, int pictureWidth, int pictureHeight);
ProxyVideoCommand buildProxyVideoCommand(const ProxyVideoRequest &request);

#endif
