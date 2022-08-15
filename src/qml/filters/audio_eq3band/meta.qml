import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Equalizer: 3-Band (Bass & Treble)")
    mlt_service: 'ladspa.1204'
    objectName: '3BandEq'
    qml: 'ui.qml'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '6', '12']
        parameters: [
            Parameter {
                name: qsTr('Equalizer')
                property: '0'
                gangedProperties: ['6', '12']
                isCurve: false
            }
        ]
    }

}
