import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr('Levels', 'Levels video filter')
    keywords: qsTr('gamma value black white', 'search keywords for the Levels video filter') + ' levels'
    mlt_service: 'frei0r.levels'
    qml: 'ui.qml'

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
