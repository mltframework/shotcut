import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: qsTr("Ambisonic Decoder")
    mlt_service: "ambisonic-decoder"
    keywords: qsTr('spatial surround', 'search keywords for the Ambisonic Decoder audio filter') + ' ambisonic decoder'
    objectName: "ambisonic-decoder"
    qml: "ui.qml"
    vui: "vui.qml"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['yaw', "pitch", "roll", "zoom"]
        parameters: [
            Parameter {
                name: qsTr('Yaw')
                property: 'yaw'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Pitch', 'rotation around the side-to-side axis (roll, pitch, yaw)')
                property: 'pitch'
                isCurve: true
                minimum: -180
                maximum: 180
            },
            Parameter {
                name: qsTr('Roll')
                property: 'roll'
                isCurve: true
                minimum: -180
                maximum: 180
            },
            Parameter {
                name: qsTr('Zoom')
                property: 'zoom'
                isCurve: true
                minimum: -100
                maximum: 100
            }
        ]
    }
}
