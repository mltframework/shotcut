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

#include "gpuinfo.h"

#include <QSet>
#include <QtGlobal>

// Platform-independent: pick the hardware encoder matching the GPU vendor, with a
// fallback to the first type-compatible encoder. Kept out of the Windows-only block
// so it builds and is unit-testable everywhere.
QString preferredHardwareVcodec(const QStringList &hardwareCodecs,
                                const QString &softwareVcodec,
                                uint vendorId,
                                bool is10bit)
{
    auto matchesType = [&softwareVcodec, is10bit](const QString &hw) {
        // No known H.264 hardware encoder supports 10-bit, so skip it in that case.
        return (softwareVcodec == QLatin1String("libx264") && hw.startsWith(QLatin1String("h264"))
                && !is10bit)
               || (softwareVcodec == QLatin1String("libx265")
                   && hw.startsWith(QLatin1String("hevc")))
               || (softwareVcodec == QLatin1String("libvpx-vp9")
                   && hw.startsWith(QLatin1String("vp9")))
               || (softwareVcodec == QLatin1String("libsvtav1")
                   && hw.startsWith(QLatin1String("av1")));
    };
    QString preferredSuffix;
    switch (vendorId) {
    case kGpuVendorNvidia:
        preferredSuffix = QStringLiteral("_nvenc");
        break;
    case kGpuVendorAmd:
        preferredSuffix = QStringLiteral("_amf");
        break;
    case kGpuVendorIntel:
        preferredSuffix = QStringLiteral("_qsv");
        break;
    default:
        break;
    }
    if (!preferredSuffix.isEmpty()) {
        for (const QString &hw : hardwareCodecs) {
            if (matchesType(hw) && hw.endsWith(preferredSuffix))
                return hw;
        }
    }
    for (const QString &hw : hardwareCodecs) {
        if (matchesType(hw))
            return hw;
    }
    return QString();
}

#ifdef Q_OS_WIN
#include <dxgi.h>

QList<GpuAdapterInfo> enumerateGpuAdapters()
{
    QList<GpuAdapterInfo> result;
    IDXGIFactory1 *factory = nullptr;
    // IID_IDXGIFactory1 is provided by the dxguid import library (linked in CMake),
    // which keeps this portable across MinGW/GCC and MSVC.
    if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void **>(&factory)))
        || !factory)
        return result;

    // Some drivers (notably AMD integrated graphics) enumerate the same physical GPU
    // many times with identical hardware ids but different LUIDs. De-duplicate by a
    // stable hardware key so each GPU appears once in the menu, while preserving the
    // true DXGI adapter index (which is what QT_D3D_ADAPTER_INDEX expects).
    QSet<QString> seen;
    IDXGIAdapter1 *adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        if (adapter) {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(adapter->GetDesc1(&desc))
                // Hide the Microsoft Basic Render Driver (software rasterizer).
                && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
                const QString key = QStringLiteral("%1:%2:%3:%4")
                                        .arg(desc.VendorId)
                                        .arg(desc.DeviceId)
                                        .arg(desc.SubSysId)
                                        .arg(desc.Revision);
                if (!seen.contains(key)) {
                    seen.insert(key);
                    GpuAdapterInfo info;
                    info.index = static_cast<int>(i);
                    info.name = QString::fromWCharArray(desc.Description);
                    info.vendorId = desc.VendorId;
                    info.deviceId = desc.DeviceId;
                    result.append(info);
                }
            }
            adapter->Release();
            adapter = nullptr;
        }
    }
    factory->Release();
    return result;
}

int gpuAdapterIndexFor(uint vendorId, uint deviceId)
{
    if (vendorId == 0)
        return -1;
    IDXGIFactory1 *factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void **>(&factory)))
        || !factory)
        return -1;
    int found = -1;
    IDXGIAdapter1 *adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        if (adapter) {
            DXGI_ADAPTER_DESC1 desc;
            if (found < 0 && SUCCEEDED(adapter->GetDesc1(&desc))
                && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && desc.VendorId == vendorId
                && desc.DeviceId == deviceId) {
                found = static_cast<int>(i);
            }
            adapter->Release();
            adapter = nullptr;
        }
    }
    factory->Release();
    return found;
}

#else // !Q_OS_WIN

QList<GpuAdapterInfo> enumerateGpuAdapters()
{
    // Index-based GPU selection is currently only implemented for the Windows
    // Direct3D RHI backend. Returning empty hides the selection UI elsewhere.
    return QList<GpuAdapterInfo>();
}

int gpuAdapterIndexFor(uint, uint)
{
    return -1;
}

#endif
