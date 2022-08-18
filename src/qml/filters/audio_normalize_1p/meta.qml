import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Normalize: One Pass")
    keywords: qsTr('volume loudness gain dynamics', 'search keywords for the Normalize: One Pass audio filter') + ' normalize: one pass'
    mlt_service: "dynamic_loudness"
    qml: "ui.qml"
}
