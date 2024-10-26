import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'reframe'
    name: qsTr('Reframe')
    keywords: qsTr('crop trim remove square vertical portrait', 'search keywords for the Reframe video filter') + ' reframe'
    mlt_service: 'mask_start'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'
    isOutputOnly: true
    isGpuCompatible: false

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['rect']
        parameters: [
            Parameter {
                name: qsTr('Position')
                property: 'rect'
                isRectangle: true
            }
        ]
    }
}
