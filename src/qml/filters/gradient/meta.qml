import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'gradient'
    name: qsTr('Gradient')
    keywords: qsTr('graduated color spectrum', 'search keywords for the Gradient video filter') + ' gradient'
    mlt_service: 'frei0r.cairogradient'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['shotcut:rect']
    }
}
