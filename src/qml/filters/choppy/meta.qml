import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Choppy')
    keywords: qsTr('fps framerate', 'search keywords for the Choppy video filter') + ' choppy'
    mlt_service: 'choppy'
    qml: 'ui.qml'
    icon: 'icon.webp'

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
