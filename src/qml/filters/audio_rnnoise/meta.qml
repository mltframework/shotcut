// Copyright (C) 2026 Meltytech, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Noise Reduction (RNNoise)")
    keywords: qsTr('noise reduction denoise background wind speech clean', 'search keywords for the Noise Reduction audio filter') + ' rnnoise'
    mlt_service: 'rnnoise'
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/noise-reduction-rnnoise-audio-filter'
}
