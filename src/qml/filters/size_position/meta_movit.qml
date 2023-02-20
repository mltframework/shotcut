import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'movitSizePosition'
    name: qsTr('Size and Position')
    keywords: qsTr('transform zoom distort fill move', 'search keywords for the Size and Position filter') + ' size position'
    mlt_service: 'movit.rect'
    needsGPU: true
    qml: 'ui_movit.qml'
    vui: 'vui_movit.qml'
    icon: 'icon.webp'
    allowMultiple: false
    isFavorite: true

    keyframes {
        allowTrim: false
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['rect']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'rect'
                isRectangle: true
            }
        ]
    }
}
