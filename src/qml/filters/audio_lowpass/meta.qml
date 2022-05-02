import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Low Pass")
    mlt_service: 'ladspa.1891'
    qml: 'ui.qml'
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['wetness']
        parameters: [
            Parameter {
                name: qsTr('Cutoff')
                property: '0'
                isCurve: true
                minimum: 5
                maximum: 21600
                units: 'Hz'
            },
            Parameter {
                name: qsTr('Rolloff rate')
                property: '1'
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
