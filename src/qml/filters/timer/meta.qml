import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'timer'
    name: qsTr('Timer')
    keywords: qsTr('text seconds timestamp', 'search keywords for the Timer video filter') + ' timer #rgba'
    mlt_service: 'timer'
    qml: "ui.qml"
    vui: 'vui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/timer/12890/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['geometry', 'fgcolour', 'olcolour', 'bgcolour', 'opacity']
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
            },
            Parameter {
                name: qsTr('Opacity')
                property: 'opacity'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
