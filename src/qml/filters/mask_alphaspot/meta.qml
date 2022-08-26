import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr('Mask: Simple Shape')
    keywords: qsTr('matte stencil alpha rectangle ellipse circle triangle diamond', 'search keywords for the Mask: Simple Shape video filter') + ' mask: simple shape'
    mlt_service: 'mask_start'
    objectName: 'maskSimpleShape'
    qml: 'ui.qml'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['filter.1', 'filter.2', 'filter.3', 'filter.4', 'filter.5']
        parameters: [
            Parameter {
                name: qsTr('Horizontal')
                property: 'filter.1'
                isCurve: true
                minimum: -1
                maximum: 1
            },
            Parameter {
                name: qsTr('Vertical')
                property: 'filter.2'
                isCurve: true
                minimum: -1
                maximum: 1
            },
            Parameter {
                name: qsTr('Width')
                property: 'filter.3'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Height')
                property: 'filter.4'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Rotation')
                property: 'filter.5'
                isCurve: true
                minimum: 0.5 - 179.9 / 360
                maximum: 0.5 + 179.9 / 360
            }
        ]
    }

}
