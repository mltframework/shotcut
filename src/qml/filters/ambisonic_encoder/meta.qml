import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Ambisonic Encoder")
    mlt_service: "ambisonic-encoder"
    keywords: qsTr('spatial surround panner', 'search keywords for the Ambisonic Encoder audio filter') + ' ambisonic encoder'
    objectName: "ambisonic-encoder"
    qml: "ui.qml"
    vui: "vui.qml"
    help: "https://forum.shotcut.org/t/ambisonic-encoder-audio-filter/50886/1"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['azimuth', 'elevation']
        parameters: [
            Parameter {
                name: qsTr('Azimuth')
                property: 'azimuth'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Elevation')
                property: 'elevation'
                isCurve: true
                minimum: -360
                maximum: 360
            }
        ]
    }
}
