import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Sketch")
    keywords: qsTr('drawing painting cartoon', 'search keywords for the Sketch video filter') + ' sketch'
    mlt_service: "charcoal"
    qml: "ui.qml"
    icon: 'icon.webp'
}
