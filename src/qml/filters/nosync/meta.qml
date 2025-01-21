import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('No Sync')
    keywords: qsTr('horizontal vertical synchronization slip analog', 'search keywords for the No Sync video filter') + ' vhs no sync'
    objectName: 'nosync'
    mlt_service: 'frei0r.nosync0r'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('V Offset')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('H Offset')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
