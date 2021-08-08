import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'dynamicText'
    name: qsTr('Text: Simple')
    mlt_service: 'dynamictext'
    qml: "ui.qml"
    vui: 'vui.qml'
    isGpuCompatible: false
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['geometry']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'geometry'
                isRectangle: true
            }
        ]
    }
}
