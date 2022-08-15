import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Reverb")
    mlt_service: 'ladspa.1216'
    qml: 'ui.qml'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '2', '3', '4', '5', '6']
        parameters: [
            Parameter {
                name: qsTr('Room size')
                property: '0'
                isCurve: true
                minimum: 1
                maximum: 300
                units: 'm'
            },
            Parameter {
                name: qsTr('Reverb time')
                property: '1'
                isCurve: true
                minimum: 0.1
                maximum: 30
                units: 's'
            },
            Parameter {
                name: qsTr('Damping')
                property: '2'
                isCurve: true
                minimum: 0
                maximum: 1
                units: '%'
            },
            Parameter {
                name: qsTr('Input bandwidth')
                property: '3'
                isCurve: true
                minimum: 0
                maximum: 1
                units: '%'
            },
            Parameter {
                name: qsTr('Dry signal level')
                property: '4'
                isCurve: true
                minimum: -70
                maximum: 0
                units: 'dB'
            },
            Parameter {
                name: qsTr('Early reflection level')
                property: '5'
                isCurve: true
                minimum: -70
                maximum: 0
                units: 'dB'
            },
            Parameter {
                name: qsTr('Tail level')
                property: '6'
                isCurve: true
                minimum: -70
                maximum: 0
                units: 'dB'
            }
        ]
    }

}
