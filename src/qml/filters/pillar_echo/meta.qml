import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'blur_pad'
    name: qsTr('Blur: Pad')
    keywords: qsTr('pillar echo fill', 'search keywords for the Blur: Pad video filter') + ' blur: pad #rgba'
    mlt_service: 'pillar_echo'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'

    keyframes {
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
