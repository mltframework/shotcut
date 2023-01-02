import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Equalizer: 3-Band (Bass & Treble)")
    keywords: qsTr('tone frequency', 'search keywords for the Equalizer: 3-Band audio filter') + ' equalizer: 3-band bass treble'
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
