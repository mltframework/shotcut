import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'gradient'
    name: qsTr('Gradient')
    mlt_service: 'frei0r.cairogradient'
    qml: 'ui.qml'
    vui: 'vui.qml'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['shotcut:rect']
    }

}
