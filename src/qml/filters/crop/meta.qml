import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Crop: Source")
    keywords: qsTr('trim remove edges', 'search keywords for the Crop: Source video filter') + ' crop: source'
    mlt_service: "crop"
    qml: "ui.qml"
    gpuAlt: "movit.crop"
    allowMultiple: false
    isClipOnly: true
}
