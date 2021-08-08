import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'cropRectangle'
    name: qsTr('Crop: Rectangle')
    mlt_service: 'qtcrop'
    qml: 'ui.qml'
    vui: 'vui.qml'
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['rect', 'radius']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'rect'
                isRectangle: true
            },
            Parameter {
                name: qsTr('Corner radius')
                property: 'radius'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
