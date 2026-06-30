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

#ifndef GPUINFO_H
#define GPUINFO_H

#include <QList>
#include <QString>
#include <QStringList>

// PCI vendor ids of the common GPU vendors.
enum GpuVendorId {
    kGpuVendorNvidia = 0x10DE,
    kGpuVendorAmd = 0x1002,
    kGpuVendorIntel = 0x8086,
};

struct GpuAdapterInfo
{
    int index = -1;    // current DXGI adapter index; matches QT_D3D_ADAPTER_INDEX
    QString name;      // human-readable description, e.g. "NVIDIA GeForce RTX 3090"
    uint vendorId = 0; // PCI vendor id (see GpuVendorId)
    uint deviceId = 0; // PCI device id; together with vendorId identifies the GPU
};

// Enumerate the physical GPU adapters usable by the renderer.
// On Windows this uses DXGI (the same enumeration order Qt's D3D RHI backend uses).
// Returns an empty list on platforms/backends where index-based selection is not
// supported, in which case the caller should hide the selection UI.
QList<GpuAdapterInfo> enumerateGpuAdapters();

// Resolve the CURRENT DXGI adapter index (as QT_D3D_ADAPTER_INDEX expects) of the GPU
// identified by vendorId+deviceId, or -1 if not found. Some drivers enumerate adapters
// in an unstable order across runs, so the index must be resolved live at startup from
// the GPU's stable identity rather than persisted from a previous session.
int gpuAdapterIndexFor(uint vendorId, uint deviceId);

// Choose the hardware video encoder for a given software codec, preferring the encoder
// family matching the selected GPU vendor (NVIDIA->*_nvenc, AMD->*_amf, Intel->*_qsv)
// and otherwise falling back to the first type-compatible encoder in hardwareCodecs.
// When is10bit is true, H.264 hardware encoders are skipped because they do not support
// 10-bit. Returns an empty string when no compatible hardware encoder is available.
// This is a pure, platform-independent function so it can be unit tested.
QString preferredHardwareVcodec(const QStringList &hardwareCodecs,
                                const QString &softwareVcodec,
                                uint vendorId,
                                bool is10bit);

#endif // GPUINFO_H
