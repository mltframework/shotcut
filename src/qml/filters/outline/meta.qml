import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Outline')
    keywords: qsTr('stroke thickness', 'search keywords for the Outlin video filter') + ' outline #rgba #10bit'
    mlt_service: 'outline'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['color', 'thickness']
        parameters: [
            Parameter {
                name: qsTr('Color')
                property: 'color'
                isCurve: false
                isColor: true
            },
            Parameter {
                name: qsTr('Thickness')
                property: 'thickness'
                isCurve: true
                minimum: 0
                maximum: 20
            }
        ]
    }
}
