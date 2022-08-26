import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Mirror")
    keywords: qsTr('horizontal flip transpose flop', 'search keywords for the Mirror video filter') + ' mirror'
    mlt_service: "movit.mirror"
    needsGPU: true
    qml: "ui.qml"
}
