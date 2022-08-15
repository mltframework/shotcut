import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr('Mask: From File')
    mlt_service: 'mask_start'
    objectName: 'maskFromFile'
    qml: 'ui.qml'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['filter.mix']
        parameters: [
            Parameter {
                name: qsTr('Threshold')
                property: 'filter.mix'
                isCurve: true
                minimum: 0
                maximum: 100
            }
        ]
    }

}
