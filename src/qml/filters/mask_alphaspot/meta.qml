import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr('Mask: Simple Shape')
    mlt_service: 'mask_start'
    objectName: 'maskSimpleShape'
    qml: 'ui.qml'
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['filter.1', 'filter.2', 'filter.3', 'filter.4']
        parameters: [
            Parameter {
                name: qsTr('Horizontal')
                property: 'filter.1'
                isSimple: true
                isCurve: true
                minimum: -1
                maximum: 1
            },
            Parameter {
                name: qsTr('Vertical')
                property: 'filter.2'
                isSimple: true
                isCurve: true
                minimum: -1
                maximum: 1
            },
            Parameter {
                name: qsTr('Width')
                property: 'filter.3'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Height')
                property: 'filter.4'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
