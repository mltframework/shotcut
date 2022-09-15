import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Old Film: %1").arg('Technocolor')
    keywords: qsTr('projector movie', 'search keywords for the Old Film: Technocolor video filter') + ' technicolor old film technocolor'
    mlt_service: "tcolor"
    qml: "ui.qml"
    icon: 'icon.qml'
}
