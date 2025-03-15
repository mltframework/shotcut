import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Old Film: %1").arg('Technocolor')
    keywords: qsTr('projector movie', 'search keywords for the Old Film: Technocolor video filter') + ' technicolor old film technocolor #yuv'
    mlt_service: "tcolor"
    qml: "ui.qml"
    icon: 'icon.webp'
}
