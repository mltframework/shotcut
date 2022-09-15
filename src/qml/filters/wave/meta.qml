import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Wave")
    keywords: qsTr('distort deform frequency water warp bend', 'search keywords for the Wave video filter') + ' wave'
    mlt_service: "wave"
    qml: "ui.qml"
    icon: 'icon.webp'
}
