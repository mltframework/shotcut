import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("White Balance")
    keywords: qsTr('color correct light temperature neutral', 'search keywords for the White Balance video filter') + ' white balance #gpu #10bit #color'
    mlt_service: "movit.white_balance"
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    help: 'https://forum.snapflow.org/t/white-balance/12894/1'
}
