import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'webvfxChoppy'
    name: qsTr('Choppy')
    mlt_service: 'webvfx'
    qml: 'ui.qml'
    isHidden: true
    isDeprecated: true
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['amount']
        parameters: [
            Parameter {
                name: qsTr('Repeat')
                property: 'amount'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 60
            }
        ]
    }
}
