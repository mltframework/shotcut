Skip to content
Your account has been flagged.
Because of that, your profile is hidden from the public. If you believe this is a mistake, contact support to have your account status reviewed.

open source video editor
Repositories31
Code4M+
Commits2K
Issues6K
Marketplace0
Topics0
Wikis4K
Users1
Languages
5 Python
3 C++
2 Java
2 JavaScript
1 C#
1 CSS
Advanced search 
31 repository results
mltframework/shotcut
cross-platform (Qt), open-source (GPLv3) video editor
gplv3
video-editor
mlt
shotcut
cross-platform
GPL-3.0 license

Updated 4 days ago

 C++
 2.1k
olive-editor/olive
Professional open-source NLE video editor
linux
macos
windows
qt
opengl
cross-platform
cpp
glsl
vst
hardware-acceleration
video-editor
GPL-3.0 license

Updated 4 days ago

 C++
 468
OpenShot/openshot-qt
OpenShot Video Editor is an award-winning free and open-source video editor for Linux, Mac, and Windows, and is dedic…
openshot
video-editing
video-production
Updated 9 days ago

1 issue needs help
 Python
 803
oracle2025/Open-Movie-Editor
Open Source Video Editing Software
GPL-2.0 license

Updated on Feb 27, 2011

 C++
 14
alessandrod/pitivi
PiTiVi open source video editor
Updated on Oct 4, 2010

 Python
 10
nragot/super-lama-video-editor-BETA
open source java application to do video editing
Apache-2.0 license

Updated on Nov 15, 2016

 Java
 3
lvm/docker-kdenlive
A container for kdenlive, a free and open-source video editor for GNU/Linux
Updated on Apr 23, 2017

 2
SkyAphid/JDialogue
Open source branching dialogue editor for video games
Updated a day ago

 Java
 10
btpoe/vide-os
Open Source video editor built with Electron and React
Apache-2.0 license

Updated on Jan 27, 2018

 JavaScript
 2
okulovsky/Tuto
Tuto editor - an open source program for the fast and easy creation of educational video.
Updated on Jul 9, 2016

 C#
© 2019 GitHub, Inc.
Terms
Privacy
Security
Status
Help
Contact GitHub
Pricing
API
Training
Blog
About
Press h to open a hovercard with more details.
Skip to content
Your account has been flagged.
Because of that, your profile is hidden from the public. If you believe this is a mistake, contact support to have your account status reviewed.

open source video editor
Repositories31
Code4M+
Commits2K
Issues6K
Marketplace0
Topics0
Wikis4K
Users1
Languages
680,360 JavaScript
655,922 PHP
509,559 HTML
433,039 Gettext Catalog
328,546 XML
228,906 Text
190,332 Unity3D Asset
112,661 JSON
108,721 Markdown
81,451 C
Advanced search 
Showing 4,086,433 available code results 
@Android4SAM
Android4SAM/platform_frameworks_av – VideoEditorMp3Reader.cpp
Showing the top four matches Last indexed on Jun 27, 2018
C++
    ALOGV("VideoEditorMp3Reader_open Datasource end");

    if (pReaderContext->mDataSource == NULL) {
        ALOGV("VideoEditorMp3Reader_open Datasource error");
    pReaderContext->mExtractor = MediaExtractor::Create(
        pReaderContext->mDataSource,MEDIA_MIMETYPE_AUDIO_MPEG);
    ALOGV("VideoEditorMp3Reader_open extractor end");
@Android4SAM
Android4SAM/platform_frameworks_av – VideoEditorAudioEncoder.cpp
Showing the top two matches Last indexed on Jun 27, 2018
C++
#define VIDEOEDITOR_FORCECODEC kSoftwareCodecsOnly

namespace android {
struct VideoEditorAudioEncoderSource : public MediaSource {
    public:
        static sp<VideoEditorAudioEncoderSource> Create(
@warpboard
warpboard/platform_frameworks_av – VideoEditorMp3Reader.cpp
Showing the top four matches Last indexed on Jun 28, 2018
C++
    ALOGV("VideoEditorMp3Reader_open Datasource end");

    if (pReaderContext->mDataSource == NULL) {
        ALOGV("VideoEditorMp3Reader_open Datasource error");
    pReaderContext->mExtractor = MediaExtractor::Create(
        pReaderContext->mDataSource,MEDIA_MIMETYPE_AUDIO_MPEG);
    ALOGV("VideoEditorMp3Reader_open extractor end");
@warpboard
warpboard/platform_frameworks_av – VideoEditorAudioEncoder.cpp
Showing the top two matches Last indexed on Jun 28, 2018
C++
#define VIDEOEDITOR_FORCECODEC kSoftwareCodecsOnly

namespace android {
struct VideoEditorAudioEncoderSource : public MediaSource {
    public:
        static sp<VideoEditorAudioEncoderSource> Create(
@MIPS
MIPS/frameworks-media-libvideoeditor – VideoEditorMp3Reader.cpp
Showing the top five matches Last indexed on Jun 26, 2018
C++
        (char*)pFileDescriptor);
    pReaderContext->mDataSource = new FileSource ((char*)pFileDescriptor);
    LOGV("VideoEditorMp3Reader_open Datasource end");

    if (pReaderContext->mDataSource == NULL) {
        LOGV("VideoEditorMp3Reader_open Datasource error");
@MIPS
MIPS/frameworks-media-libvideoeditor – VideoEditorAudioEncoder.cpp
Showing the top three matches Last indexed on Jun 26, 2018
C++
#include "VideoEditorUtils.h"

#include "utils/Log.h"
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MediaDebug.h>
#define VIDEOEDITOR_FORCECODEC kSoftwareCodecsOnly

namespace android {
struct VideoEditorAudioEncoderSource : public MediaSource {
@AOSP-S4-KK
AOSP-S4-KK/platform_frameworks_av – VideoEditorMp3Reader.cpp
Showing the top four matches Last indexed on Jun 27, 2018
C++
    ALOGV("VideoEditorMp3Reader_open Datasource end");

    if (pReaderContext->mDataSource == NULL) {
        ALOGV("VideoEditorMp3Reader_open Datasource error");
    pReaderContext->mExtractor = MediaExtractor::Create(
        pReaderContext->mDataSource,MEDIA_MIMETYPE_AUDIO_MPEG);
    ALOGV("VideoEditorMp3Reader_open extractor end");
@steppnasty
steppnasty/platform_frameworks_av – VideoEditorMp3Reader.cpp
Showing the top four matches Last indexed on Jun 27, 2018
C++
    ALOGV("VideoEditorMp3Reader_open Datasource end");

    if (pReaderContext->mDataSource == NULL) {
        ALOGV("VideoEditorMp3Reader_open Datasource error");
    pReaderContext->mExtractor = MediaExtractor::Create(
        pReaderContext->mDataSource,MEDIA_MIMETYPE_AUDIO_MPEG);
    ALOGV("VideoEditorMp3Reader_open extractor end");
@DennisBold
DennisBold/android_frameworks_av – VideoEditorMp3Reader.cpp
Showing the top four matches Last indexed on Jun 27, 2018
C++
    ALOGV("VideoEditorMp3Reader_open Datasource end");

    if (pReaderContext->mDataSource == NULL) {
        ALOGV("VideoEditorMp3Reader_open Datasource error");
    pReaderContext->mExtractor = MediaExtractor::Create(
        pReaderContext->mDataSource,MEDIA_MIMETYPE_AUDIO_MPEG);
    ALOGV("VideoEditorMp3Reader_open extractor end");
@friendlyarm
friendlyarm/android_frameworks_av – VideoEditorMp3Reader.cpp
Showing the top four matches Last indexed on Jul 1, 2018
C++
    ALOGV("VideoEditorMp3Reader_open Datasource end");

    if (pReaderContext->mDataSource == NULL) {
        ALOGV("VideoEditorMp3Reader_open Datasource error");
    pReaderContext->mExtractor = MediaExtractor::Create(
        pReaderContext->mDataSource,MEDIA_MIMETYPE_AUDIO_MPEG);
    ALOGV("VideoEditorMp3Reader_open extractor end");
© 2019 GitHub, Inc.
Terms
Privacy
Security
Status
Help
Contact GitHub
Pricing
API
Training
Blog
About
Press h to open a hovercard with more details.
