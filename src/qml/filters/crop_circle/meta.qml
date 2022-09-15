import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'cropCircle'
    name: qsTr('Crop: Circle')
    keywords: qsTr('trim remove oval ellipse', 'search keywords for the Crop: Circle video filter') + ' crop: circle'
    mlt_service: 'qtcrop'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['radius']
        parameters: [
            Parameter {
                name: qsTr('Radius')
                property: 'radius'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }

}
