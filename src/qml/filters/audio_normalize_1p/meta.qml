import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Normalize: One Pass")
    keywords: qsTr('volume loudness gain dynamics', 'search keywords for the Normalize: One Pass audio filter') + ' normalize: one pass'
    mlt_service: "dynamic_loudness"
    qml: "ui.qml"
    help: 'https://forum.shotcut.org/t/normalize-one-pass-audio-filter/12911/1'
}
