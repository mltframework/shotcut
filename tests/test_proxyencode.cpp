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

#include "proxyencode.h"

#include <QtTest/QtTest>

namespace {

const QString kResource = QStringLiteral("/clips/in.mov");
const QString kOutput = QStringLiteral("/proxies/out.pending.mp4");

// Same condition as h264VideoToolboxEncoder(). This machine is Apple Silicon.
QStringList videoToolboxRate()
{
#if defined(Q_OS_MAC) && defined(Q_PROCESSOR_ARM)
    return {QStringLiteral("-q:v"), QStringLiteral("45")};
#else
    return {QStringLiteral("-b:v"), QStringLiteral("8M")};
#endif
}

ProxyVideoRequest baseRequest()
{
    ProxyVideoRequest request;
    request.resource = kResource;
    request.outputFile = kOutput;
    request.resolution = 540;
    request.videoIndex = 0;
    request.audioIndex = 1;
    request.hardware = true;
    return request;
}

QString videoFilter(const QStringList &args)
{
    const int at = args.indexOf(QStringLiteral("-vf"));
    if (at < 0 || at + 1 >= args.size())
        return QString();
    return args.at(at + 1);
}

QString sequenceError(const QStringList &args, const QStringList &sequence)
{
    int from = 0;
    for (const QString &token : sequence) {
        const int at = args.indexOf(token, from);
        if (at < from)
            return QStringLiteral("missing \"%1\" in %2").arg(token, args.join(' '));
        from = at + 1;
    }
    return QString();
}

QString absentError(const QStringList &args, const QStringList &tokens)
{
    for (const QString &token : tokens) {
        if (args.contains(token))
            return QStringLiteral("unexpected \"%1\" in %2").arg(token, args.join(' '));
    }
    return QString();
}

QString tailError(const QStringList &args, const QString &gop)
{
    if (args.size() < 6)
        return QStringLiteral("argv shorter than the GOP tail");
    const QStringList tail = args.mid(args.size() - 6);
    const QStringList expected{QStringLiteral("-g"),
                               gop,
                               QStringLiteral("-bf"),
                               QStringLiteral("0"),
                               QStringLiteral("-y"),
                               kOutput};
    if (tail != expected)
        return QStringLiteral("tail %1 != %2").arg(tail.join(' '), expected.join(' '));
    return QString();
}

} // namespace

// QFAIL records the failure on the running test slot before returning here.
void checkSequence(const QStringList &args, const QStringList &sequence)
{
    const QString error = sequenceError(args, sequence);
    if (!error.isEmpty())
        QFAIL(qPrintable(error));
}

void checkAbsent(const QStringList &args, const QStringList &tokens)
{
    const QString error = absentError(args, tokens);
    if (!error.isEmpty())
        QFAIL(qPrintable(error));
}

void checkTail(const QStringList &args, const QString &gop)
{
    const QString error = tailError(args, gop);
    if (!error.isEmpty())
        QFAIL(qPrintable(error));
}

class TestProxyEncode : public QObject
{
    Q_OBJECT

private slots:
    void macProgressiveHardwareUsesVideoToolbox()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Mac;
        request.scanMode = kProxyScanProgressive;
        request.hardwareCodecs = {QStringLiteral("hevc_videotoolbox"), QStringLiteral("hevc_nvenc")};

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        checkSequence(command.args,
                      QStringList({QStringLiteral("-hwaccel"),
                                   QStringLiteral("videotoolbox"),
                                   QStringLiteral("-hwaccel_output_format"),
                                   QStringLiteral("videotoolbox_vld"),
                                   QStringLiteral("-map"),
                                   QStringLiteral("0:V?"),
                                   QStringLiteral("-map"),
                                   QStringLiteral("0:a?")}));
        const QString filter = videoFilter(command.args);
        QVERIFY(filter.contains(QStringLiteral("scale_vt=w=-2:h=540")));
        QVERIFY(filter.contains(QStringLiteral(
            "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709:range=tv")));
        QVERIFY(!filter.contains(QStringLiteral("yadif")));
        QVERIFY(!filter.contains(QStringLiteral("scale=width=")));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("videotoolbox_vld"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_videotoolbox")})
                          + videoToolboxRate());
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-colorspace"),
                                 QStringLiteral("-color_range"),
                                 QStringLiteral("-bsf:v"),
                                 QStringLiteral("hevc_videotoolbox"),
                                 QStringLiteral("hevc_nvenc"),
                                 QStringLiteral("-hide_banner")}));
        checkTail(command.args, QStringLiteral("1"));

        // The software retry is assembled on its own. Deleting hwaccel tokens
        // from the GPU argv would leave scale_vt in the filter.
        QVERIFY(!command.fallbackArgs.isEmpty());
        checkAbsent(command.fallbackArgs,
                    QStringList({QStringLiteral("-hwaccel"),
                                 QStringLiteral("-hwaccel_output_format"),
                                 QStringLiteral("videotoolbox"),
                                 QStringLiteral("videotoolbox_vld")}));
        const QString fallbackFilter = videoFilter(command.fallbackArgs);
        QCOMPARE(fallbackFilter,
                 QStringLiteral("scale=width=-2:height=540:in_range=mpeg:out_range=mpeg"));
        QVERIFY(!fallbackFilter.contains(QStringLiteral("yadif")));
        QVERIFY(!fallbackFilter.contains(QStringLiteral("scale_vt")));
        checkSequence(command.fallbackArgs,
                      QStringList({QStringLiteral("-color_range"),
                                   QStringLiteral("mpeg"),
                                   QStringLiteral("-pix_fmt"),
                                   QStringLiteral("yuv420p"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_videotoolbox")})
                          + videoToolboxRate());
        checkTail(command.fallbackArgs, QStringLiteral("1"));
    }

    void macAutomaticHardwareDeinterlacesOnGpu()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Mac;
        request.scanMode = kProxyScanAutomatic;

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(videoFilter(command.args)
                    .startsWith(QStringLiteral("yadif_videotoolbox=deint=interlaced,")));
        const QString fallbackFilter = videoFilter(command.fallbackArgs);
        QVERIFY(fallbackFilter.startsWith(
            QStringLiteral("yadif=deint=interlaced,scale=width=-2:height=540")));
        QVERIFY(!fallbackFilter.contains(QStringLiteral("scale_vt")));
        checkAbsent(command.fallbackArgs, QStringList({QStringLiteral("-hwaccel")}));
    }

    void macExplicitInterlaceSkipsGpu()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Mac;
        request.scanMode = kProxyScanTopFieldFirst;
        request.hardwareCodecs = {QStringLiteral("hevc_nvenc"), QStringLiteral("hevc_videotoolbox")};

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(command.fallbackArgs.isEmpty());
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-hwaccel"),
                                 QStringLiteral("videotoolbox_vld"),
                                 QStringLiteral("hevc_nvenc"),
                                 QStringLiteral("hevc_videotoolbox")}));
        QCOMPARE(videoFilter(command.args),
                 QStringLiteral(
                     "yadif=parity=tff,scale=width=-2:height=540:in_range=mpeg:out_range=mpeg"));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("yuv420p"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_videotoolbox")})
                          + videoToolboxRate());
        checkTail(command.args, QStringLiteral("1"));
    }

    void macFullRangeStillUsesVideoToolbox()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Mac;
        request.scanMode = kProxyScanProgressive;
        request.fullRange = true;
        request.hardwareCodecs = {QStringLiteral("hevc_videotoolbox")};

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(videoFilter(command.args).contains(QStringLiteral("range=pc")));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"), QStringLiteral("h264_videotoolbox")})
                          + videoToolboxRate()
                          + QStringList({QStringLiteral("-bsf:v"),
                                         QStringLiteral("h264_metadata=video_full_range_flag=1")}));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-color_range"), QStringLiteral("libx264")}));
        QCOMPARE(videoFilter(command.fallbackArgs),
                 QStringLiteral("scale=width=-2:height=540:in_range=full:out_range=full"));
        checkSequence(command.fallbackArgs,
                      QStringList({QStringLiteral("-color_range"),
                                   QStringLiteral("jpeg"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_videotoolbox")})
                          + videoToolboxRate());
        checkAbsent(command.fallbackArgs, QStringList({QStringLiteral("-bsf:v")}));
    }

    void windowsSurfaceFitsUsesD3d11()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Windows;
        request.scanMode = kProxyScanAutomatic;
        request.codecName = QStringLiteral("h264");
        request.pictureWidth = 1920;
        request.pictureHeight = 1080;
        request.hardwareCodecs = {QStringLiteral("hevc_nvenc")};

        QVERIFY(
            proxyD3d11Surface(request.codecName, request.pictureWidth, request.pictureHeight).fits);

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        checkSequence(command.args,
                      QStringList({QStringLiteral("-hwaccel"),
                                   QStringLiteral("d3d11va"),
                                   QStringLiteral("-hwaccel_output_format"),
                                   QStringLiteral("d3d11")}));
        QCOMPARE(videoFilter(command.args),
                 QStringLiteral("scale_d3d11=width=trunc(oh*a/2)*2:height=540:format=nv12,"
                                "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709:"
                                "range=tv"));
        QVERIFY(!videoFilter(command.args).contains(QStringLiteral("yadif")));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("d3d11"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_mf"),
                                   QStringLiteral("-rate_control"),
                                   QStringLiteral("quality"),
                                   QStringLiteral("-quality"),
                                   QStringLiteral("41"),
                                   QStringLiteral("-hw_encoding"),
                                   QStringLiteral("true")}));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("hevc_nvenc"), QStringLiteral("-colorspace")}));
        checkTail(command.args, QStringLiteral("1"));

        checkAbsent(command.fallbackArgs,
                    QStringList({QStringLiteral("-hwaccel"),
                                 QStringLiteral("d3d11va"),
                                 QStringLiteral("d3d11")}));
        QCOMPARE(videoFilter(command.fallbackArgs),
                 QStringLiteral("yadif=deint=interlaced,"
                                "scale=width=-2:height=540:in_range=mpeg:out_range=mpeg"));
        QVERIFY(!videoFilter(command.fallbackArgs).contains(QStringLiteral("scale_d3d11")));
        checkSequence(command.fallbackArgs,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("nv12"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_mf")}));
        checkTail(command.fallbackArgs, QStringLiteral("1"));
    }

    void windowsSurfaceTooLargeUsesSoftwareScale()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Windows;
        request.scanMode = kProxyScanAutomatic;
        request.codecName = QStringLiteral("hevc");
        request.pictureWidth = 2704;
        request.pictureHeight = 1520;
        request.hardwareCodecs = {QStringLiteral("hevc_nvenc")};

        QVERIFY(
            !proxyD3d11Surface(request.codecName, request.pictureWidth, request.pictureHeight).fits);

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(command.fallbackArgs.isEmpty());
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-hwaccel"),
                                 QStringLiteral("d3d11va"),
                                 QStringLiteral("d3d11"),
                                 QStringLiteral("hevc_nvenc")}));
        QVERIFY(videoFilter(command.args).contains(QStringLiteral("scale=width=-2:height=540")));
        QVERIFY(!videoFilter(command.args).contains(QStringLiteral("scale_d3d11")));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("nv12"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_mf")}));
        checkTail(command.args, QStringLiteral("1"));
    }

    void windowsExplicitInterlaceUsesListedEncoder()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Windows;
        request.scanMode = kProxyScanTopFieldFirst;
        request.codecName = QStringLiteral("h264");
        request.pictureWidth = 1920;
        request.pictureHeight = 1080;
        request.hardwareCodecs = {QStringLiteral("h264_nvenc")};

        QVERIFY(
            proxyD3d11Surface(request.codecName, request.pictureWidth, request.pictureHeight).fits);

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(command.fallbackArgs.isEmpty());
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-hwaccel"),
                                 QStringLiteral("d3d11va"),
                                 QStringLiteral("h264_mf")}));
        QCOMPARE(videoFilter(command.args),
                 QStringLiteral(
                     "yadif=parity=tff,scale=width=-2:height=540:in_range=mpeg:out_range=mpeg"));
        QVERIFY(!videoFilter(command.args).contains(QStringLiteral("scale_d3d11")));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("yuv420p"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_nvenc"),
                                   QStringLiteral("-rc"),
                                   QStringLiteral("constqp"),
                                   QStringLiteral("-qp:v"),
                                   QStringLiteral("37"),
                                   QStringLiteral("-b_ref_mode"),
                                   QStringLiteral("0")}));
        checkTail(command.args, QStringLiteral("2"));
    }

    void windowsSoftwareScaleKeepsVaapiUpload()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Windows;
        request.codecName = QStringLiteral("hevc");
        request.pictureWidth = 2704;
        request.pictureHeight = 1520;
        request.hardwareCodecs = {QStringLiteral("hevc_vaapi")};

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(videoFilter(command.args).endsWith(QStringLiteral(",format=nv12,hwupload")));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"), QStringLiteral("h264_mf")}));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("hevc_vaapi"), QStringLiteral("-hwaccel")}));
    }

    void h264VaapiUsesHardwareTranscode()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Generic;
        request.scanMode = kProxyScanProgressive;
        // Higher-priority codecs stay in the list. h264_vaapi still selects
        // the VAAPI graph.
        request.hardwareCodecs = {QStringLiteral("hevc_nvenc"),
                                  QStringLiteral("hevc_vaapi"),
                                  QStringLiteral("h264_vaapi")};

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        checkSequence(command.args,
                      QStringList({QStringLiteral("-hwaccel"),
                                   QStringLiteral("vaapi"),
                                   QStringLiteral("-hwaccel_output_format"),
                                   QStringLiteral("vaapi")}));
        QCOMPARE(videoFilter(command.args),
                 QStringLiteral("scale_vaapi=w=-2:h=540:format=nv12,"
                                "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709:"
                                "range=tv"));
        QVERIFY(!videoFilter(command.args).contains(QStringLiteral("deinterlace_vaapi")));
        QVERIFY(!videoFilter(command.args).contains(QStringLiteral("hwupload")));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("vaapi"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_vaapi"),
                                   QStringLiteral("-qp"),
                                   QStringLiteral("30")}));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("hevc_nvenc"),
                                 QStringLiteral("hevc_vaapi"),
                                 QStringLiteral("-init_hw_device"),
                                 QStringLiteral("-colorspace"),
                                 QStringLiteral("-bsf:v")}));
        checkTail(command.args, QStringLiteral("2"));

        checkAbsent(command.fallbackArgs,
                    QStringList({QStringLiteral("-hwaccel"),
                                 QStringLiteral("scale_vaapi"),
                                 QStringLiteral("hevc_nvenc"),
                                 QStringLiteral("hevc_vaapi")}));
        QCOMPARE(videoFilter(command.fallbackArgs),
                 QStringLiteral("scale=width=-2:height=540:in_range=mpeg:out_range=mpeg,"
                                "format=nv12,hwupload"));
        checkSequence(command.fallbackArgs,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("yuv420p"),
                                   QStringLiteral("-init_hw_device"),
                                   QStringLiteral("vaapi=vaapi0:"),
                                   QStringLiteral("-filter_hw_device"),
                                   QStringLiteral("vaapi0"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_vaapi"),
                                   QStringLiteral("-qp"),
                                   QStringLiteral("30")}));
        checkTail(command.fallbackArgs, QStringLiteral("1"));
    }

    void h264VaapiAutomaticFallbackDeinterlaces()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Generic;
        request.scanMode = kProxyScanAutomatic;
        request.hardwareCodecs = {QStringLiteral("h264_vaapi")};

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(!videoFilter(command.args).contains(QStringLiteral("yadif")));
        QVERIFY(!videoFilter(command.args).contains(QStringLiteral("deinterlace_vaapi")));
        QVERIFY(videoFilter(command.fallbackArgs)
                    .startsWith(QStringLiteral("yadif=deint=interlaced,scale=width=-2:height=540")));
        QVERIFY(videoFilter(command.fallbackArgs).endsWith(QStringLiteral(",format=nv12,hwupload")));
    }

    void h264VaapiFullRangeMarksPc()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Generic;
        request.scanMode = kProxyScanProgressive;
        request.fullRange = true;
        request.hardwareCodecs = {QStringLiteral("h264_vaapi")};

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(videoFilter(command.args).contains(QStringLiteral("range=pc")));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-bsf:v"), QStringLiteral("-color_range")}));
        QCOMPARE(videoFilter(command.fallbackArgs),
                 QStringLiteral("scale=width=-2:height=540:in_range=full:out_range=full,"
                                "format=nv12,hwupload"));
        checkSequence(command.fallbackArgs,
                      QStringList({QStringLiteral("-color_range"), QStringLiteral("jpeg")}));
    }

    void h264VaapiExplicitInterlaceStaysOnYadif()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Generic;
        request.scanMode = kProxyScanTopFieldFirst;
        request.hardwareCodecs = {QStringLiteral("h264_vaapi")};

        const ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(command.fallbackArgs.isEmpty());
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-hwaccel"), QStringLiteral("scale_vaapi")}));
        QCOMPARE(videoFilter(command.args),
                 QStringLiteral("yadif=parity=tff,"
                                "scale=width=-2:height=540:in_range=mpeg:out_range=mpeg,"
                                "format=nv12,hwupload"));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"), QStringLiteral("h264_vaapi")}));
        checkTail(command.args, QStringLiteral("1"));
    }

    void hevcVaapiKeepsSoftwareUpload()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Generic;
        request.scanMode = kProxyScanProgressive;
        request.hardwareCodecs = {QStringLiteral("hevc_vaapi")};

        ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(command.fallbackArgs.isEmpty());
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-hwaccel"),
                                 QStringLiteral("scale_vaapi"),
                                 QStringLiteral("h264_vaapi")}));
        QCOMPARE(videoFilter(command.args),
                 QStringLiteral("scale=width=-2:height=540:in_range=mpeg:out_range=mpeg,"
                                "format=nv12,hwupload"));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-init_hw_device"),
                                   QStringLiteral("vaapi=vaapi0:"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("hevc_vaapi"),
                                   QStringLiteral("-qp"),
                                   QStringLiteral("37")}));
        checkTail(command.args, QStringLiteral("1"));

        request.hardwareCodecs = {QStringLiteral("hevc_vaapi"), QStringLiteral("h264_nvenc")};
        command = buildProxyVideoCommand(request);
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"), QStringLiteral("hevc_vaapi")}));
        checkAbsent(command.args, QStringList({QStringLiteral("h264_nvenc")}));
        checkTail(command.args, QStringLiteral("1"));
    }

    void nvencKeepsSoftwareEncode()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Generic;
        request.scanMode = kProxyScanProgressive;
        request.hardwareCodecs = {QStringLiteral("h264_nvenc")};

        ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(command.fallbackArgs.isEmpty());
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-hwaccel"),
                                 QStringLiteral("hwupload"),
                                 QStringLiteral("h264_vaapi"),
                                 QStringLiteral("scale_vaapi")}));
        QCOMPARE(videoFilter(command.args),
                 QStringLiteral("scale=width=-2:height=540:in_range=mpeg:out_range=mpeg"));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_nvenc"),
                                   QStringLiteral("-qp:v"),
                                   QStringLiteral("37")}));
        checkTail(command.args, QStringLiteral("2"));

        request.hardwareCodecs = {QStringLiteral("hevc_nvenc")};
        command = buildProxyVideoCommand(request);
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"), QStringLiteral("hevc_nvenc")}));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("-hwaccel"), QStringLiteral("h264_vaapi")}));
        checkTail(command.args, QStringLiteral("0"));
    }

    void windowsAndMacIgnoreH264VaapiList()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Windows;
        request.scanMode = kProxyScanProgressive;
        request.codecName = QStringLiteral("h264");
        request.pictureWidth = 1920;
        request.pictureHeight = 1080;
        request.hardwareCodecs = {QStringLiteral("h264_vaapi")};

        ProxyVideoCommand command = buildProxyVideoCommand(request);
        checkSequence(command.args,
                      QStringList({QStringLiteral("-hwaccel"), QStringLiteral("d3d11va")}));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"), QStringLiteral("h264_mf")}));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("vaapi"), QStringLiteral("h264_vaapi")}));

        request.policy = ProxyHwPolicy::Mac;
        command = buildProxyVideoCommand(request);
        checkSequence(command.args,
                      QStringList({QStringLiteral("-hwaccel"), QStringLiteral("videotoolbox")}));
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"),
                                   QStringLiteral("h264_videotoolbox")}));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("scale_vaapi"), QStringLiteral("h264_vaapi")}));
    }

    void hardwareOffIgnoresListedNvenc()
    {
        ProxyVideoRequest request = baseRequest();
        request.policy = ProxyHwPolicy::Generic;
        request.hardware = false;
        request.hardwareCodecs = {QStringLiteral("hevc_nvenc")};

        ProxyVideoCommand command = buildProxyVideoCommand(request);
        QVERIFY(command.fallbackArgs.isEmpty());
        checkSequence(command.args,
                      QStringList({QStringLiteral("-pix_fmt"),
                                   QStringLiteral("yuv420p"),
                                   QStringLiteral("-codec:v"),
                                   QStringLiteral("libx264"),
                                   QStringLiteral("-preset"),
                                   QStringLiteral("veryfast"),
                                   QStringLiteral("-crf"),
                                   QStringLiteral("23")}));
        checkAbsent(command.args,
                    QStringList({QStringLiteral("hevc_nvenc"), QStringLiteral("-hwaccel")}));
        checkTail(command.args, QStringLiteral("1"));

        request.hardwareCodecs = {QStringLiteral("h264_nvenc")};
        command = buildProxyVideoCommand(request);
        checkSequence(command.args,
                      QStringList({QStringLiteral("-codec:v"), QStringLiteral("libx264")}));
        checkAbsent(command.args, QStringList({QStringLiteral("h264_nvenc")}));
        checkTail(command.args, QStringLiteral("1"));
    }
};

QTEST_APPLESS_MAIN(TestProxyEncode)

#include "test_proxyencode.moc"
