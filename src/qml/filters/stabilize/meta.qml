import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Stabilize")
    keywords: qsTr('smooth deshake', 'search keywords for the Stabilize video filter') + ' vid.stab stabilize #yuv'
    mlt_service: "vidstab"
    qml: "ui.qml"
    icon: 'icon.webp'
    isClipOnly: true
    allowMultiple: false

    keyframes {
        allowTrim: false
    }
}
