import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Invert Colors")
    keywords: qsTr('reverse opposite negative', 'search keywords for the Invert Colors video filter') + ' invert colors #yuv'
    mlt_service: "invert"
    qml: "ui.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/invert-colors/12853/1'
}
