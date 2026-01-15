import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Equalizer: 15-Band")
    keywords: qsTr('tone frequency', 'search keywords for the Equalizer: 15-Band audio filter') + ' equalizer: 15-band'
    mlt_service: 'ladspa.1197'
    objectName: '15BandEq'
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/equalizer-15-band-audio-filter/32466/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14']
        parameters: [
            Parameter {
                name: qsTr('Equalizer')
                property: '0'
                gangedProperties: ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14']
                isCurve: false
            }
        ]
    }
}
