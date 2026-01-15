import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Blend Mode")
    keywords: qsTr('blending composite porter duff', 'search keywords for the Blend Mode video filter') + ' blend mode #rgba'
    mlt_service: "cairoblend_mode"
    objectName: 'blendMode'
    qml: "ui_cairoblend.qml"
    icon: 'icon.webp'
    isClipOnly: true
    allowMultiple: false
    isGpuCompatible: false
    isHidden: true
    help: 'https://forum.shotcut.org/t/blend-mode/12841/1'
}
