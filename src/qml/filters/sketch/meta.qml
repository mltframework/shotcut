import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Sketch")
    keywords: qsTr('drawing painting cartoon', 'search keywords for the Sketch video filter') + ' sketch'
    mlt_service: "charcoal"
    qml: "ui.qml"
    icon: 'icon.webp'
}
