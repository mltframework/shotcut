import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Noise Gate")
    keywords: qsTr('hum hiss distortion clean', 'search keywords for the Noise Gate audio filter') + ' noise gate'
    mlt_service: 'ladspa.1410'
    qml: 'ui.qml'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '3', '4', '5', '6']
        parameters: [
            Parameter {
                name: qsTr('Key Filter: Low Frequency')
                property: '0'
                isCurve: true
                minimum: 33.6
                maximum: 4800
            },
            Parameter {
                name: qsTr('Key Filter: High Frequency')
                property: '1'
                isCurve: true
                minimum: 240
                maximum: 23520
            },
            Parameter {
                name: qsTr('Threshold')
                property: '2'
                isCurve: true
                minimum: -70
                maximum: 20
            },
            Parameter {
                name: qsTr('Attack')
                property: '3'
                isCurve: true
                minimum: 0.01
                maximum: 1000
            },
            Parameter {
                name: qsTr('Hold')
                property: '4'
                isCurve: true
                minimum: 2
                maximum: 2000
            },
            Parameter {
                name: qsTr('Decay')
                property: '5'
                isCurve: true
                minimum: 2
                maximum: 4000
            },
            Parameter {
                name: qsTr('Range')
                property: '6'
                isCurve: true
                minimum: -90
                maximum: 0
            }
        ]
    }

}
