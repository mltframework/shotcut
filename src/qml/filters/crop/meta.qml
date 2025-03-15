import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Crop: Source")
    keywords: qsTr('trim remove edges', 'search keywords for the Crop: Source video filter') + ' crop: source #rgba #yuv'
    mlt_service: "crop"
    qml: "ui.qml"
    icon: 'icon.webp'
    gpuAlt: "movit.crop"
    allowMultiple: false
    isClipOnly: true
}
