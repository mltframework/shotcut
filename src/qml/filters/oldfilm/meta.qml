import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Old Film: Projector")
    keywords: qsTr('glitch flashing brightness vertical slip', 'search keywords for the Old Film: Projector video filter') + ' old film: projector #yuv'
    mlt_service: "oldfilm"
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.snapflow.org/t/old-film-projector/12868/1'
}
