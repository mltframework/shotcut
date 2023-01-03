import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'dynamicText'
    name: qsTr('Text: Simple')
    keywords: qsTr('type font timecode timestamp date filename', 'search keywords for the Text: Simple video filter') + ' text: simple'
    mlt_service: 'dynamictext'
    qml: "ui.qml"
    vui: 'vui.qml'
    icon: 'icon.webp'
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
