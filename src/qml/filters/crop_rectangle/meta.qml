import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'cropRectangle'
    name: qsTr('Crop: Rectangle')
    keywords: qsTr('trim remove square', 'search keywords for the Crop: Rectangle video filter') + ' crop: rectangle'
    mlt_service: 'qtcrop'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'

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
