import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'movitSizePosition'
    name: qsTr('Size and Position')
    mlt_service: 'movit.rect'
    needsGPU: true
    qml: 'ui_movit.qml'
    vui: 'vui_movit.qml'
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
