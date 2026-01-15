import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'affineSizePosition'
    name: qsTr('Size, Position & Rotate')
    keywords: qsTr('transform zoom rotation distort fill move', 'search keywords for the Size, Position & Rotate video filter') + ' size position rotate #rgba #10bit'
    mlt_service: 'affine'
    qml: 'ui_affine.qml'
    vui: 'vui_affine.qml'
    icon: 'icon.webp'
    isFavorite: true
    help: 'https://forum.shotcut.org/t/size-position-rotate/12881/1'

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
