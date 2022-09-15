import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Grid")
    keywords: qsTr('repeat', 'search keywords for the Grid video filter') + ' grid'
    mlt_service: "frei0r.cairoimagegrid"
    qml: "ui.qml"
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('Rows')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Columns')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }

}
