import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Blend Mode")
    keywords: qsTr('blending composite porter duff', 'search keywords for the Blend Mode video filter') + ' blend mode #rgba'
    mlt_service: "qtblend_mode"
    objectName: 'qtBlendMode'
    qml: "ui_qtblend.qml"
    icon: 'icon.webp'
    isClipOnly: true
    allowMultiple: false
    isGpuCompatible: false
    isHidden: true
}
