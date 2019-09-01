import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Blend Mode")
    mlt_service: "cairoblend_mode"
    objectName: 'blendMode'
    qml: "ui.qml"
    isClipOnly: true
}
