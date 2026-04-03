import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('NTSC-rs')
    mlt_service: 'openfx.wtf.vala^NtscRs'
    keywords: qsTr('analog noise vintage tape', 'search keywords for the NTSC-rs video filter') + 'VHS VCR NTSC #openfx'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['1', '4', '6', '14', '17', '22']
        parameters: [
            Parameter {
                name: qsTr('Composite Signal Sharpening')
                property: '1'
                isCurve: true
                minimum: -1
                maximum: 2
            },
            Parameter {
                name: qsTr('Composite Noise Intensity')
                property: '4'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Snow')
                property: '6'
                isCurve: true
                minimum: 0
                maximum: 100
            },
            Parameter {
                name: qsTr('Head Switching Shift')
                property: '14'
                isCurve: true
                minimum: -100
                maximum: 100
            },
            Parameter {
                name: qsTr('Tracking Wave Intensity')
                property: '17'
                isCurve: true
                minimum: -50
                maximum: 50
            },
            Parameter {
                name: qsTr('Ringing Scale')
                property: '22'
                isCurve: true
                minimum: 0
                maximum: 10
            }
        ]
    }
}
