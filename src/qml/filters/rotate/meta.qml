import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: "affineRotate"
    name: qsTr("Rotate and Scale")
    mlt_service: "affine"
    qml: "ui.qml"
    isFavorite: true
    isHidden: true
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['transition.fix_rotate_x', 'transition.scale_x']
        minimumVersion: '3'
        parameters: [
            Parameter {
                name: qsTr('Rotation')
                property: 'transition.fix_rotate_x'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Scale')
                property: 'transition.scale_x'
                gangedProperties: ['transition.scale_y']
                isCurve: true
                minimum: 0.001
                maximum: 10
            },
            Parameter {
                name: qsTr('X offset')
                property: 'transition.ox'
            },
            Parameter {
                name: qsTr('Y offset')
                property: 'transition.oy'
            }
        ]
    }
}
