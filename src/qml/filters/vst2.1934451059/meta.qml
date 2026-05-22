import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    isAudio: true
    name: 'Valhalla Supermassive'
    mlt_service: 'vst2.1934451059'
    keywords: qsTr('reverb delay modulation', 'search keywords for filter') + ' valhalla supermassive vst2.1934451059'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: [
            '1', '2', '3', '4', '5', '6', '7',
            '8', '9', '10', '11', '12', '13', 'wetness'
        ]
        parameters: [
            Parameter {
                name: qsTr('Delay sync')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Delay note')
                property: '2'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Delay (ms)')
                property: '3'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Delay warp')
                property: '4'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Clear')
                property: '5'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Feedback')
                property: '6'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Density')
                property: '7'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Width')
                property: '8'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Low cut')
                property: '9'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('High cut')
                property: '10'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Mod rate')
                property: '11'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Mod depth')
                property: '12'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Mode')
                property: '13'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Wet/Dry')
                property: 'wetness'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
