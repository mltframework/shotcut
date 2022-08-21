import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Blend Mode")
    keywords: qsTr('blending composite porter duff', 'search keywords for the Blend Mode video filter') + ' blend mode'
    mlt_service: "cairoblend_mode"
    objectName: 'blendMode'
    qml: "ui.qml"
    isClipOnly: true
}
