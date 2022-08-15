import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Stabilize")
    mlt_service: "vidstab"
    qml: "ui.qml"
    isClipOnly: true
    allowMultiple: false
    isGpuCompatible: false

    keyframes {
        allowTrim: false
    }

}
