import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("White Balance")
    keywords: qsTr('color correct light temperature neutral', 'search keywords for the White Balance video filter') + ' white balance #rgba #color'
    mlt_service: "frei0r.colgate"
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    gpuAlt: "movit.white_balance"
    help: 'https://forum.shotcut.org/t/white-balance/12894/1'
}
