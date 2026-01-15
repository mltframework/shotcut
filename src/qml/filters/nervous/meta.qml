import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Nervous')
    keywords: qsTr('random shake twitch glitch', 'search keywords for the Nervous video filter') + ' nervous #rgba #yuv #10bit'
    mlt_service: 'avfilter.random'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/nervous-video-filter/14175/1'
}
