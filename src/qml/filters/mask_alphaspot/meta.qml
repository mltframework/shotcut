import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Mask: Simple Shape')
    keywords: qsTr('matte stencil alpha rectangle ellipse circle triangle diamond', 'search keywords for the Mask: Simple Shape video filter') + ' mask: simple shape #rgba #10bit'
    mlt_service: 'mask_start'
    objectName: 'maskSimpleShape'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/mask-simple-shape/12861/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['filter.1', 'filter.2', 'filter.3', 'filter.4', 'filter.5', 'shotcut:rect']
        parameters: [
            Parameter {
                name: qsTr('Size & Position')
                property: 'shotcut:rect'
                isRectangle: true
                gangedProperties: ['filter.1', 'filter.2', 'filter.3', 'filter.4']
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
