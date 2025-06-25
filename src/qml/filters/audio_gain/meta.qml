import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Gain / Volume")
    mlt_service: "volume"
    objectName: "audioGain"
    keywords: qsTr('loudness', 'search keywords for the Gain/Volume audio filter') + ' gain volume'
    qml: "ui.qml"
    icon: 'qrc:///icons/oxygen/32x32/status/audio-volume-high.png'
    isFavorite: true

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['level']
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'level'
                isCurve: true
                minimum: -70
                maximum: 24
            }
        ]
    }
}
