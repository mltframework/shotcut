import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'rectangle'
    name: qsTr('Rectangle')
    isHidden: true
    mlt_service: 'webvfx'
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
                isSimple: true
                isRectangle: true
            },
            Parameter {
                name: qsTr('Corner radius')
                property: 'radius'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
