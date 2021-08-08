import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr('Choppy')
    mlt_service: 'choppy'
    qml: 'ui.qml'
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['amount']
        parameters: [
            Parameter {
                name: qsTr('Repeat')
                property: 'amount'
                isCurve: true
                minimum: 0
                maximum: 60
            }
        ]
    }
}
