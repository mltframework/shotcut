import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Crop: Source")
    keywords: qsTr('trim remove edges', 'search keywords for the Crop: Source video filter') + ' crop: source #gpu'
    objectName: "movit.crop"
    mlt_service: 'crop'
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'
    allowMultiple: false
}
