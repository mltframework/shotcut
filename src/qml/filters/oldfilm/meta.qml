import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Old Film: Projector")
    keywords: qsTr('glitch flashing brightness vertical slip', 'search keywords for the Old Film: Projector video filter') + ' old film: projector'
    mlt_service: "oldfilm"
    qml: "ui.qml"
    icon: 'icon.qml'
}
