import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Equalizer: Parametric")
    keywords: qsTr('tone frequency', 'search keywords for the Equalizer: Parametric audio filter') + ' equalizer: parametric'
    mlt_service: 'ladspa.1204'
    objectName: 'parametricEq'
    qml: 'ui.qml'

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
