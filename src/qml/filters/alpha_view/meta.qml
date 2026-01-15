import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Alpha Channel: View")
    keywords: qsTr('transparency', 'search keywords for the Alpha Channel: View video filter') + ' alpha channel: view #rgba'
    mlt_service: 'frei0r.alpha0ps'
    objectName: 'alphaChannelView'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/alpha-channel-view/12820/1'
}
