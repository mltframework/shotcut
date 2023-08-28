import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Notch")
    keywords: qsTr('frequency pass', 'search keywords for the Notch audio filter') + ' notch'
    mlt_service: 'ladspa.1894'
    qml: 'ui.qml'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '2', 'wetness']
        parameters: [
            Parameter {
                name: qsTr('Center Frequency')
                property: '0'
                isCurve: true
                minimum: 5
                maximum: 21600
                units: 'Hz'
            },
            Parameter {
                name: qsTr('Bandwidth')
                property: '1'
                isCurve: true
                minimum: 5
                maximum: 21600
                units: 'Hz'
            },
            Parameter {
                name: qsTr('Rolloff rate')
                property: '2'
                isCurve: true
                minimum: 1
                maximum: 10
            },
            Parameter {
                name: qsTr('Wetness')
                property: 'wetness'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
