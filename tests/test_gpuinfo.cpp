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
#include <QtTest/QtTest>

class TestGpuInfo : public QObject
{
    Q_OBJECT

private slots:
    // ---- preferredHardwareVcodec (pure logic) ----------------------------------

    void prefersVendorFamily()
    {
        const QStringList hw{"h264_nvenc", "h264_amf", "hevc_nvenc", "hevc_amf"};
        QCOMPARE(preferredHardwareVcodec(hw, "libx264", kGpuVendorNvidia, false),
                 QString("h264_nvenc"));
        QCOMPARE(preferredHardwareVcodec(hw, "libx265", kGpuVendorNvidia, false),
                 QString("hevc_nvenc"));
        QCOMPARE(preferredHardwareVcodec(hw, "libx264", kGpuVendorAmd, false),
                 QString("h264_amf"));
        QCOMPARE(preferredHardwareVcodec(hw, "libx265", kGpuVendorAmd, false),
                 QString("hevc_amf"));
    }

    void prefersQsvForIntel()
    {
        const QStringList hw{"h264_nvenc", "h264_qsv"};
        QCOMPARE(preferredHardwareVcodec(hw, "libx264", kGpuVendorIntel, false),
                 QString("h264_qsv"));
    }

    void orderIndependent()
    {
        // The NVIDIA encoder must win even when the AMD one comes first in the list.
        const QStringList hw{"h264_amf", "h264_nvenc"};
        QCOMPARE(preferredHardwareVcodec(hw, "libx264", kGpuVendorNvidia, false),
                 QString("h264_nvenc"));
    }

    void fallsBackToFirstWhenFamilyMissing()
    {
        // NVIDIA selected but only an AMF encoder is available -> use it.
        const QStringList hw{"h264_amf"};
        QCOMPARE(preferredHardwareVcodec(hw, "libx264", kGpuVendorNvidia, false),
                 QString("h264_amf"));
    }

    void fallsBackToFirstWhenAutomatic()
    {
        // vendorId 0 (Automatic) -> first type-compatible encoder.
        const QStringList hw{"h264_amf", "h264_nvenc"};
        QCOMPARE(preferredHardwareVcodec(hw, "libx264", 0u, false), QString("h264_amf"));
    }

    void matchesCodecType()
    {
        const QStringList hw{"h264_nvenc", "hevc_nvenc", "av1_nvenc", "vp9_qsv"};
        QCOMPARE(preferredHardwareVcodec(hw, "libsvtav1", kGpuVendorNvidia, false),
                 QString("av1_nvenc"));
        // No vp9 NVENC exists -> fall back to the first vp9 encoder.
        QCOMPARE(preferredHardwareVcodec(hw, "libvpx-vp9", kGpuVendorNvidia, false),
                 QString("vp9_qsv"));
    }

    void skipsH264HardwareFor10bit()
    {
        // No H.264 hardware encoder supports 10-bit, so it must not be selected; HEVC
        // hardware encoders are unaffected (they do support 10-bit).
        const QStringList hw{"h264_nvenc", "hevc_nvenc"};
        QVERIFY(preferredHardwareVcodec(hw, "libx264", kGpuVendorNvidia, true).isEmpty());
        QCOMPARE(preferredHardwareVcodec(hw, "libx265", kGpuVendorNvidia, true),
                 QString("hevc_nvenc"));
    }

    void returnsEmptyWhenNoneCompatible()
    {
        const QStringList hw{"h264_nvenc", "hevc_nvenc"};
        QVERIFY(preferredHardwareVcodec(hw, "libvpx-vp9", kGpuVendorNvidia, false).isEmpty());
        QVERIFY(preferredHardwareVcodec(QStringList(), "libx264", kGpuVendorNvidia, false).isEmpty());
    }

    // ---- enumerateGpuAdapters / gpuAdapterIndexFor (hardware dependent) ---------
    // These assert invariants only; on a machine/CI without a usable GPU the list is
    // empty and the loops are simply skipped.

    void enumerationInvariants()
    {
        const QList<GpuAdapterInfo> adapters = enumerateGpuAdapters();
        QSet<QString> identities;
        for (const GpuAdapterInfo &g : adapters) {
            QVERIFY2(g.index >= 0, "adapter index must be non-negative");
            QVERIFY2(g.vendorId != 0, "adapter vendorId must be set");
            QVERIFY2(!g.name.isEmpty(), "adapter name must be set");
            // De-duplication: each physical GPU appears at most once.
            const QString id = QStringLiteral("%1:%2").arg(g.vendorId).arg(g.deviceId);
            QVERIFY2(!identities.contains(id), "adapters must be de-duplicated by identity");
            identities.insert(id);
        }
    }

    void indexResolution()
    {
        const QList<GpuAdapterInfo> adapters = enumerateGpuAdapters();
        for (const GpuAdapterInfo &g : adapters) {
            // A present GPU resolves to a valid live index...
            QVERIFY(gpuAdapterIndexFor(g.vendorId, g.deviceId) >= 0);
        }
        // ...an absent GPU and the "automatic" sentinel resolve to -1.
        QCOMPARE(gpuAdapterIndexFor(0xFFFFu, 0xFFFFu), -1);
        QCOMPARE(gpuAdapterIndexFor(0u, 0u), -1);
    }
};

QTEST_MAIN(TestGpuInfo)
#include "test_gpuinfo.moc"
