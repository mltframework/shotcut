import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("White Balance")
    keywords: qsTr('color correct light temperature neutral', 'search keywords for the White Balance video filter') + ' white balance'
    mlt_service: "frei0r.colgate"
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    gpuAlt: "movit.white_balance"
}
