import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('No Sync')
    keywords: qsTr('vertical synchronization slip analog', 'search keywords for the No Sync video filter') + ' vhs no sync'
    objectName: 'nosync'
    mlt_service: 'frei0r.nosync0r'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0']
        parameters: [
            Parameter {
                name: qsTr('Offset')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
