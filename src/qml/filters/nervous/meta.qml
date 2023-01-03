import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Nervous')
    keywords: qsTr('random shake twitch glitch', 'search keywords for the Nervous video filter') + ' nervous'
    mlt_service: 'avfilter.random'
    qml: 'ui.qml'
    icon: 'icon.webp'
}
