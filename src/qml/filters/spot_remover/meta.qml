import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'spotRemover'
    name: qsTr('Spot Remover')
    keywords: qsTr('delogo dirt clean watermark', 'search keywords for the Spot Remover video filter') + ' spot remover'
    mlt_service: 'spot_remover'
    qml: 'ui.qml'
    vui: 'vui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['rect']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'rect'
                isRectangle: true
            }
        ]
    }

}
