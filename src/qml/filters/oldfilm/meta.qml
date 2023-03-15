import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Old Film: Projector")
    keywords: qsTr('glitch flashing brightness vertical slip', 'search keywords for the Old Film: Projector video filter') + ' old film: projector'
    mlt_service: "oldfilm"
    qml: "ui.qml"
    icon: 'icon.webp'
}
