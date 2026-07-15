import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'shake'
    name: qsTr('Shake')
    keywords: qsTr('hand-held camera tremble vibrate', 'search keywords for the Shake video filter') + ' shake #addon #rgba'
    mlt_service: 'frei0r.camerashake'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '2', '3', '4', '5', '6', '7']
        parameters: [
            Parameter {
                name: qsTr('Amplitude X')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Amplitude Y')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Rotation')
                property: '2'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Zoom')
                property: '3'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Speed')
                property: '4'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Opacity')
                property: '5'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Blur')
                property: '6'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Background color')
                property: '7'
                isCurve: false
                isColor: true
            }
        ]
    }
}
