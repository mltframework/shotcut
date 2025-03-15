import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("White Balance")
    keywords: qsTr('color correct light temperature neutral', 'search keywords for the White Balance video filter') + ' white balance #gpu'
    mlt_service: "movit.white_balance"
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
}
