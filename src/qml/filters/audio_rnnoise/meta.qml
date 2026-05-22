// Copyright (C) 2026 Meltytech, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Reduce Noise: Audio (RNNoise)")
    keywords: qsTr('reduce noise denoise background wind speech clean', 'search keywords for the Track Reduce Audio Noise filter') + 'Track Reduce Audio Noise rnnoise'
    mlt_service: 'rnnoise'
    qml: 'ui.qml'
    isTrackOnly: true
}
