import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Normalize: Two Pass")
    mlt_service: "loudness"
    keywords: qsTr('volume loudness gain dynamics', 'search keywords for the Normalize: Two Pass audio filter') + ' normalize: two pass'
    qml: "ui.qml"
    isClipOnly: true
    allowMultiple: false

    keyframes {
        allowTrim: false
    }
}
