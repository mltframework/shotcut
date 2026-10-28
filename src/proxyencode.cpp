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

struct ColorTags
{
    QString primaries;
    QString trc;
    QString space;
    bool setTrc = true;
};

struct ProxyPiece
{
    QStringList beforeInput;
    QString filter;
    QStringList tail;
    bool fixedGop = false;
};

static bool explicitInterlace(int scanMode)
{
    return scanMode == kProxyScanTopFieldFirst || scanMode == kProxyScanBottomFieldFirst;
}

static int d3d11SurfaceAlignment(const QString &codecName)
{
    if (codecName == QLatin1String("hevc") || codecName == QLatin1String("av1")
        || codecName == QLatin1String("libdav1d") || codecName == QLatin1String("libaom-av1"))
        return 128;
    if (codecName == QLatin1String("mpeg2video"))
        return 32;
    return 16;
}

static int alignD3d11Surface(int value, int alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

ProxyHwPolicy nativeProxyHwPolicy()
{
#if defined(Q_OS_WIN)
    return ProxyHwPolicy::Windows;
#elif defined(Q_OS_MAC)
    return ProxyHwPolicy::Mac;
#else
    return ProxyHwPolicy::Generic;
#endif
}

ProxyD3d11Surface proxyD3d11Surface(const QString &codecName, int pictureWidth, int pictureHeight)
{
    ProxyD3d11Surface surface;
    const int alignment = d3d11SurfaceAlignment(codecName);
    if (pictureWidth <= 0 || pictureHeight <= 0 || alignment <= 0)
        return surface;
    surface.alignedWidth = alignD3d11Surface(pictureWidth, alignment);
    surface.alignedHeight = alignD3d11Surface(pictureHeight, alignment);
    const int padX = surface.alignedWidth - pictureWidth;
    const int padY = surface.alignedHeight - pictureHeight;
    surface.fits = padX <= 16 && padY <= 16;
    return surface;
}

static ColorTags colorTagsFor(int colorspace, int mediaHeight)
{
    ColorTags tags;
    tags.primaries = QStringLiteral("bt709");
    tags.trc = QStringLiteral("bt709");
    tags.space = QStringLiteral("bt709");
    switch (colorspace) {
    case 601:
        if (mediaHeight == 576) {
            tags.primaries = QStringLiteral("bt470bg");
            tags.trc = QStringLiteral("smpte170m");
            tags.space = QStringLiteral("bt470bg");
        } else {
            tags.primaries = QStringLiteral("smpte170m");
            tags.trc = QStringLiteral("smpte170m");
            tags.space = QStringLiteral("smpte170m");
        }
        break;
    case 170:
        tags.primaries = QStringLiteral("smpte170m");
        tags.trc = QStringLiteral("smpte170m");
        tags.space = QStringLiteral("smpte170m");
        break;
    case 240:
        tags.primaries = QStringLiteral("smpte240m");
        tags.trc = QStringLiteral("smpte240m");
        tags.space = QStringLiteral("smpte240m");
        break;
    case 470:
        tags.primaries = QStringLiteral("bt470bg");
        tags.trc = QStringLiteral("bt470bg");
        tags.space = QStringLiteral("bt470bg");
        break;
    case 2020:
        tags.primaries = QStringLiteral("bt2020");
        tags.space = QStringLiteral("bt2020nc");
        tags.setTrc = false;
        break;
    default:
        break;
    }
    return tags;
}

static QString setparamsFilter(const ColorTags &tags, bool fullRange)
{
    const QString range = fullRange ? QStringLiteral("pc") : QStringLiteral("tv");
    if (tags.setTrc) {
        return QStringLiteral("setparams=color_primaries=%1:color_trc=%2:colorspace=%3:range=%4")
            .arg(tags.primaries, tags.trc, tags.space, range);
    }
    return QStringLiteral("setparams=color_primaries=%1:colorspace=%2:range=%3")
        .arg(tags.primaries, tags.space, range);
}

static QString softwareDeinterlace(int scanMode)
{
    if (scanMode == kProxyScanAutomatic)
        return QStringLiteral("yadif=deint=interlaced,");
    if (scanMode == kProxyScanTopFieldFirst)
        return QStringLiteral("yadif=parity=tff,");
    if (scanMode == kProxyScanBottomFieldFirst)
        return QStringLiteral("yadif=parity=bff,");
    return QString();
}

static QString fullRangeSuffix(bool fullRange)
{
    return fullRange ? QStringLiteral(":in_range=full:out_range=full")
                     : QStringLiteral(":in_range=mpeg:out_range=mpeg");
}

static QString softwareScaleFilter(const ProxyVideoRequest &request, bool vaapiUpload)
{
    QString filters = softwareDeinterlace(request.scanMode);
    filters += QStringLiteral("scale=width=-2:height=%1").arg(request.resolution);
    filters += fullRangeSuffix(request.fullRange);
    if (vaapiUpload)
        filters += QStringLiteral(",format=nv12,hwupload");
    return filters;
}

static QStringList colorArgs(const ColorTags &tags, bool fullRange)
{
    QStringList args;
    args << QStringLiteral("-color_range")
         << (fullRange ? QStringLiteral("jpeg") : QStringLiteral("mpeg"));
    args << QStringLiteral("-color_primaries") << tags.primaries;
    if (tags.setTrc)
        args << QStringLiteral("-color_trc") << tags.trc;
    args << QStringLiteral("-colorspace") << tags.space;
    return args;
}

static QStringList containerAndAudio(const ProxyVideoRequest &request)
{
    QStringList args;
    if (!request.aspectRatio.isNull()) {
        args << QStringLiteral("-aspect")
             << QStringLiteral("%1:%2").arg(request.aspectRatio.x()).arg(request.aspectRatio.y());
    }
    args << QStringLiteral("-f") << QStringLiteral("mp4");
    args << QStringLiteral("-codec:a") << QStringLiteral("ac3");
    args << QStringLiteral("-b:a") << QStringLiteral("256k");
    return args;
}

static QStringList h264MfEncoder(bool d3d11Frames)
{
    QStringList args;
    args << QStringLiteral("-pix_fmt")
         << (d3d11Frames ? QStringLiteral("d3d11") : QStringLiteral("nv12"));
    args << QStringLiteral("-codec:v") << QStringLiteral("h264_mf");
    args << QStringLiteral("-rate_control") << QStringLiteral("quality");
    args << QStringLiteral("-quality") << QStringLiteral("41");
    args << QStringLiteral("-hw_encoding") << QStringLiteral("true");
    return args;
}

static QStringList h264VideoToolboxEncoder(bool hardwareFrames, bool fullRange)
{
    QStringList args;
    args << QStringLiteral("-pix_fmt")
         << (hardwareFrames ? QStringLiteral("videotoolbox_vld") : QStringLiteral("yuv420p"));
    args << QStringLiteral("-codec:v") << QStringLiteral("h264_videotoolbox");
    // Constant quality is only implemented for VideoToolbox on Apple Silicon.
#if defined(Q_OS_MAC) && defined(Q_PROCESSOR_ARM)
    args << QStringLiteral("-q:v") << QStringLiteral("45");
#else
    args << QStringLiteral("-b:v") << QStringLiteral("8M");
#endif
    // Hardware frames keep the decoder's video-range pixel buffer, so
    // VideoToolbox writes video_full_range_flag=0. setparams still marks the
    // colr atom full, which is what MediaInfo reads. MLT reads the VUI flag.
    if (hardwareFrames && fullRange) {
        args << QStringLiteral("-bsf:v") << QStringLiteral("h264_metadata=video_full_range_flag=1");
    }
    return args;
}

static QStringList listedHardwareEncoder(const QStringList &hwCodecs)
{
    QStringList args;
    if (hwCodecs.contains(QLatin1String("hevc_nvenc"))) {
        args << QStringLiteral("-codec:v") << QStringLiteral("hevc_nvenc");
        args << QStringLiteral("-rc") << QStringLiteral("constqp");
        args << QStringLiteral("-qp:v") << QStringLiteral("37");
        args << QStringLiteral("-b_ref_mode") << QStringLiteral("0");
    } else if (hwCodecs.contains(QLatin1String("hevc_qsv"))) {
        args << QStringLiteral("-load_plugin") << QStringLiteral("hevc_hw");
        args << QStringLiteral("-codec:v") << QStringLiteral("hevc_qsv");
        args << QStringLiteral("-q:v") << QStringLiteral("36");
    } else if (hwCodecs.contains(QLatin1String("hevc_amf"))) {
        args << QStringLiteral("-codec:v") << QStringLiteral("hevc_amf");
        args << QStringLiteral("-rc") << QStringLiteral("1");
        args << QStringLiteral("-qp_i") << QStringLiteral("32");
        args << QStringLiteral("-qp_p") << QStringLiteral("32");
    } else if (hwCodecs.contains(QLatin1String("hevc_vaapi"))) {
        args << QStringLiteral("-init_hw_device") << QStringLiteral("vaapi=vaapi0:");
        args << QStringLiteral("-filter_hw_device") << QStringLiteral("vaapi0");
        args << QStringLiteral("-codec:v") << QStringLiteral("hevc_vaapi");
        args << QStringLiteral("-qp") << QStringLiteral("37");
    } else if (hwCodecs.contains(QLatin1String("hevc_mf"))) {
        args << QStringLiteral("-codec:v") << QStringLiteral("hevc_mf");
        args << QStringLiteral("-rate_control") << QStringLiteral("quality");
        args << QStringLiteral("-quality") << QStringLiteral("27");
        args << QStringLiteral("-hw_encoding") << QStringLiteral("true");
        args << QStringLiteral("-pix_fmt") << QStringLiteral("nv12");
    } else if (hwCodecs.contains(QLatin1String("h264_nvenc"))) {
        args << QStringLiteral("-codec:v") << QStringLiteral("h264_nvenc");
        args << QStringLiteral("-rc") << QStringLiteral("constqp");
        args << QStringLiteral("-qp:v") << QStringLiteral("37");
        args << QStringLiteral("-b_ref_mode") << QStringLiteral("0");
    } else if (hwCodecs.contains(QLatin1String("h264_vaapi"))) {
        args << QStringLiteral("-init_hw_device") << QStringLiteral("vaapi=vaapi0:");
        args << QStringLiteral("-filter_hw_device") << QStringLiteral("vaapi0");
        args << QStringLiteral("-codec:v") << QStringLiteral("h264_vaapi");
        args << QStringLiteral("-qp") << QStringLiteral("30");
    } else if (hwCodecs.contains(QLatin1String("h264_qsv"))) {
        args << QStringLiteral("-codec:v") << QStringLiteral("h264_qsv");
        args << QStringLiteral("-q:v") << QStringLiteral("36");
    } else if (hwCodecs.contains(QLatin1String("h264_amf"))) {
        args << QStringLiteral("-codec:v") << QStringLiteral("h264_amf");
        args << QStringLiteral("-rc") << QStringLiteral("1");
        args << QStringLiteral("-qp_i") << QStringLiteral("32");
        args << QStringLiteral("-qp_p") << QStringLiteral("32");
    } else if (hwCodecs.contains(QLatin1String("h264_mf"))) {
        args << QStringLiteral("-codec:v") << QStringLiteral("h264_mf");
        args << QStringLiteral("-rate_control") << QStringLiteral("quality");
        args << QStringLiteral("-quality") << QStringLiteral("41");
        args << QStringLiteral("-hw_encoding") << QStringLiteral("true");
        args << QStringLiteral("-pix_fmt") << QStringLiteral("nv12");
    }
    return args;
}

static bool wantsVaapiUpload(const ProxyVideoRequest &request)
{
    return request.hardware
           && (request.hardwareCodecs.contains(QLatin1String("hevc_vaapi"))
               || request.hardwareCodecs.contains(QLatin1String("h264_vaapi")));
}

static ProxyPiece softwarePiece(const ProxyVideoRequest &request,
                                const ColorTags &tags,
                                const QStringList &encoder,
                                bool vaapiUpload,
                                bool fixedGop)
{
    ProxyPiece piece;
    piece.filter = softwareScaleFilter(request, vaapiUpload);
    piece.tail = colorArgs(tags, request.fullRange);
    piece.tail << containerAndAudio(request);
    piece.tail << encoder;
    piece.fixedGop = fixedGop;
    return piece;
}

static ProxyPiece d3d11Piece(const ProxyVideoRequest &request, const ColorTags &tags)
{
    ProxyPiece piece;
    piece.beforeInput << QStringLiteral("-hwaccel") << QStringLiteral("d3d11va");
    piece.beforeInput << QStringLiteral("-hwaccel_output_format") << QStringLiteral("d3d11");
    piece.filter = QStringLiteral("scale_d3d11=width=trunc(oh*a/2)*2:height=%1:format=nv12,%2")
                       .arg(request.resolution)
                       .arg(setparamsFilter(tags, request.fullRange));
    piece.tail = containerAndAudio(request);
    piece.tail << h264MfEncoder(true);
    piece.fixedGop = true;
    return piece;
}

static ProxyPiece videoToolboxPiece(const ProxyVideoRequest &request, const ColorTags &tags)
{
    ProxyPiece piece;
    piece.beforeInput << QStringLiteral("-hwaccel") << QStringLiteral("videotoolbox");
    piece.beforeInput << QStringLiteral("-hwaccel_output_format")
                      << QStringLiteral("videotoolbox_vld");
    if (request.scanMode == kProxyScanAutomatic)
        piece.filter = QStringLiteral("yadif_videotoolbox=deint=interlaced,");
    piece.filter += QStringLiteral("scale_vt=w=-2:h=%1,%2")
                        .arg(request.resolution)
                        .arg(setparamsFilter(tags, request.fullRange));
    piece.tail = containerAndAudio(request);
    piece.tail << h264VideoToolboxEncoder(true, request.fullRange);
    piece.fixedGop = true;
    return piece;
}

static ProxyPiece genericPiece(const ProxyVideoRequest &request, const ColorTags &tags)
{
    QStringList encoder;
    encoder << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");
    if (request.hardware)
        encoder << listedHardwareEncoder(request.hardwareCodecs);
    return softwarePiece(request, tags, encoder, wantsVaapiUpload(request), false);
}

static QStringList assemble(const ProxyVideoRequest &request, const ProxyPiece &piece)
{
    QStringList args;
    args << QStringLiteral("-loglevel") << QStringLiteral("verbose");
    args << piece.beforeInput;
    args << QStringLiteral("-noautorotate") << QStringLiteral("-i") << request.resource;
    args << QStringLiteral("-max_muxing_queue_size") << QStringLiteral("9999");
    if (request.videoIndex < request.audioIndex) {
        args << QStringLiteral("-map") << QStringLiteral("0:V?");
        if (request.iPhoneAmbisonic)
            args << QStringLiteral("-map") << QStringLiteral("0:a:0");
        else
            args << QStringLiteral("-map") << QStringLiteral("0:a?");
    } else {
        args << QStringLiteral("-map") << QStringLiteral("0:a?");
        args << QStringLiteral("-map") << QStringLiteral("0:V?");
    }
    args << QStringLiteral("-map_metadata") << QStringLiteral("0");
    args << QStringLiteral("-ignore_unknown");
    args << QStringLiteral("-vf") << piece.filter;
    args << piece.tail;
    if (!args.contains(QStringLiteral("-codec:v"))) {
        args << QStringLiteral("-codec:v") << QStringLiteral("libx264");
        args << QStringLiteral("-preset") << QStringLiteral("veryfast");
        args << QStringLiteral("-crf") << QStringLiteral("23");
    }
    QString gop = QStringLiteral("1");
    if (!piece.fixedGop) {
        if (request.hardwareCodecs.contains(QLatin1String("hevc_nvenc")))
            gop = QStringLiteral("0");
        else if (request.hardwareCodecs.contains(QLatin1String("h264_nvenc")))
            gop = QStringLiteral("2");
    }
    args << QStringLiteral("-g") << gop;
    args << QStringLiteral("-bf") << QStringLiteral("0");
    args << QStringLiteral("-y") << request.outputFile;
    return args;
}

ProxyVideoCommand buildProxyVideoCommand(const ProxyVideoRequest &request)
{
    ProxyHwPolicy policy = request.policy;
    if (policy == ProxyHwPolicy::Native)
        policy = nativeProxyHwPolicy();

    const ColorTags tags = colorTagsFor(request.colorspace, request.mediaHeight);
    const bool gpuScan = request.hardware && !explicitInterlace(request.scanMode);
    ProxyPiece primary;
    ProxyPiece fallback;
    bool hasFallback = false;

    if (policy == ProxyHwPolicy::Windows && gpuScan) {
        const ProxyD3d11Surface surface = proxyD3d11Surface(request.codecName,
                                                            request.pictureWidth,
                                                            request.pictureHeight);
        if (surface.fits) {
            primary = d3d11Piece(request, tags);
            fallback = softwarePiece(request, tags, h264MfEncoder(false), false, true);
            hasFallback = true;
        } else {
            primary = softwarePiece(request,
                                    tags,
                                    h264MfEncoder(false),
                                    wantsVaapiUpload(request),
                                    true);
        }
    } else if (policy == ProxyHwPolicy::Mac && gpuScan) {
        primary = videoToolboxPiece(request, tags);
        fallback = softwarePiece(request,
                                 tags,
                                 h264VideoToolboxEncoder(false, request.fullRange),
                                 false,
                                 true);
        hasFallback = true;
    } else if (policy == ProxyHwPolicy::Mac && request.hardware) {
        primary = softwarePiece(request,
                                tags,
                                h264VideoToolboxEncoder(false, request.fullRange),
                                false,
                                true);
    } else {
        primary = genericPiece(request, tags);
    }

    ProxyVideoCommand command;
    command.args = assemble(request, primary);
    if (hasFallback)
        command.fallbackArgs = assemble(request, fallback);
    return command;
}
