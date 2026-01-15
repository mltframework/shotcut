import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Pitch", 'audio pitch or tone')
    mlt_service: "rbpitch"
    keywords: qsTr('frequency tone', 'search keywords for the Pitch audio filter') + ' pitch rubberband'
    qml: "ui.qml"
    isFavorite: false
    help: 'https://forum.shotcut.org/t/pitch-audio-filter/15649/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['octaveshift']
        parameters: [
            Parameter {
                name: qsTr('Pitch', 'audio pitch or tone')
                property: 'octaveshift'
                isCurve: true
                minimum: -2
                maximum: 2
            }
        ]
    }
}
