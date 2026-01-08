import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Blend Mode")
    keywords: qsTr('blending composite porter duff', 'search keywords for the Blend Mode video filter') + ' blend mode #gpu #10bit'
    mlt_service: "movit.overlay_mode"
    objectName: 'movitBlendMode'
    qml: "ui_movit.qml"
    icon: 'icon.webp'
    isClipOnly: true
    allowMultiple: false
    isGpuCompatible: true
    isHidden: true
}
