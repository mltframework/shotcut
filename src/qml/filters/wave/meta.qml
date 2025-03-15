import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Wave")
    keywords: qsTr('distort deform frequency water warp bend', 'search keywords for the Wave video filter') + ' wave #yuv'
    mlt_service: "wave"
    qml: "ui.qml"
    icon: 'icon.webp'
}
