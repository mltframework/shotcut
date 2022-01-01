import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("15 Band Equalizer")
    mlt_service: 'ladspa.1197'
    qml: 'ui.qml'
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14']
        parameters: [
            Parameter {
                name: qsTr('Equalizer')
                property: '0'
                gangedProperties: ['1', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14']
                isSimple: true
                isCurve: false
            }
        ]
    }
}
