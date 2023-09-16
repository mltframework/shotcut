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

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['geometry', 'fgcolour', 'olcolour', 'bgcolour']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'geometry'
                isRectangle: true
            },
            Parameter {
                name: qsTr('Font color')
                property: 'fgcolour'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Outline')
                property: 'olcolour'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Background')
                property: 'bgcolour'
                isCurve: false
                isColor: true
            }
        ]
    }
}
