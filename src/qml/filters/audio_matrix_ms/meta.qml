import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr('Mid-Side Matrix')
    keywords: qsTr('middle stereo microphone', 'search keywords for the Mid-Side Matrix audio filter') + ' mid side matrix'
    mlt_service: 'ladspa.1421'
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/mid-side-matrix-audio-filter/50888/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', 'wetness']
        parameters: [
            Parameter {
                name: qsTr('Width')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 2
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
