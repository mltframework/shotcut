import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'timer'
    name: qsTr('Timer')
    keywords: qsTr('text seconds timestamp', 'search keywords for the Timer video filter') + ' timer'
    mlt_service: 'timer'
    qml: "ui.qml"
    vui: 'vui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['geometry']
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
            },
            Parameter {
                name: qsTr('Outline')
                property: 'olcolour'
                isCurve: false
            },
            Parameter {
                name: qsTr('Background')
                property: 'bgcolour'
                isCurve: false
            }
        ]
    }
}
