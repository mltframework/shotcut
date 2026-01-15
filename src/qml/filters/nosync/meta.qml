import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('No Sync')
    keywords: qsTr('horizontal vertical synchronization slip analog', 'search keywords for the No Sync video filter') + ' vhs no sync #rgba'
    objectName: 'nosync'
    mlt_service: 'frei0r.nosync0r'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/no-sync-video-filter/14176/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('Vertical')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Horizontal')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
