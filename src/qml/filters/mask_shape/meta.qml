import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Mask: From File')
    keywords: qsTr('matte stencil alpha luma wipe custom', 'search keywords for the Mask: From File video filter') + ' mask: from file #rgba #10bit'
    mlt_service: 'mask_start'
    objectName: 'maskFromFile'
    isGpuCompatible: false
    qml: 'ui.qml'
    icon: 'icon.webp'

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
