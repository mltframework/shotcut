import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'gradient'
    name: qsTr('Gradient')
    keywords: qsTr('graduated color spectrum', 'search keywords for the Gradient video filter') + ' gradient #rgba'
    mlt_service: 'frei0r.cairogradient'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/gradient-video-filter/14508/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['shotcut:rect']
    }
}
