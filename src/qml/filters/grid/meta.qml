import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Grid")
    mlt_service: "frei0r.cairoimagegrid"
    qml: "ui.qml"
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
