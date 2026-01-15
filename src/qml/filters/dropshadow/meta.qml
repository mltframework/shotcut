import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Drop Shadow')
    keywords: qsTr('', 'search keywords for the Drop Shadow video filter') + ' drop shadow #rgba #10bit'
    mlt_service: 'dropshadow'
    icon: 'icon.webp'
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/drop-shadow-video-filter/46273/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['color', 'radius', 'x', 'y']
        parameters: [
            Parameter {
                name: qsTr('Color')
                property: 'color'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Radius')
                property: 'radius'
                isCurve: true
                minimum: 0
                maximum: 100
            },
            Parameter {
                name: qsTr('X')
                property: 'x'
                isCurve: true
                minimum: -100
                maximum: 100
            },
            Parameter {
                name: qsTr('Y')
                property: 'y'
                isCurve: true
                minimum: -100
                maximum: 100
            }
        ]
    }
}
