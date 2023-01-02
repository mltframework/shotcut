import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'affineSizePosition'
    name: qsTr('Size, Position & Rotate')
    keywords: qsTr('transform zoom rotation distort fill move', 'search keywords for the Size, Position & Rotate video filter') + ' size position rotate'
    mlt_service: 'affine'
    qml: 'ui_affine.qml'
    vui: 'vui_affine.qml'
    icon: 'icon.webp'
    gpuAlt: 'movit.rect'
    isFavorite: true

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['transition.rect', 'transition.fix_rotate_x']
        parameters: [
            Parameter {
                name: qsTr('Size & Position')
                property: 'transition.rect'
                isRectangle: true
            },
            Parameter {
                name: qsTr('Rotation')
                property: 'transition.fix_rotate_x'
                isCurve: true
                minimum: -360
                maximum: 360
            }
        ]
    }
}
