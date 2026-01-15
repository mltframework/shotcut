import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Levels', 'Levels video filter')
    keywords: qsTr('gamma value black white color', 'search keywords for the Levels video filter') + ' levels #rgba #color'
    mlt_service: 'frei0r.levels'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/levels-video-filter/12857/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['1', '2', '3']
        parameters: [
            Parameter {
                name: qsTr('Input Black')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Input White')
                property: '2'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Gamma')
                property: '3'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
