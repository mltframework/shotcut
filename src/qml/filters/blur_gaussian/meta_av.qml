import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Blur: Gaussian")
    keywords: qsTr('soften obscure hide', 'search keywords for the Blur: Box video filter') + ' blur: gaussian #rgba #yuv #10bit'
    objectName: 'blur_gaussian_av'
    mlt_service: "avfilter.gblur"
    qml: "ui_av.qml"
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/blur-gaussian/12830/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['av.sigma', 'av.sigmaV']
        parameters: [
            Parameter {
                name: qsTr('Amount')
                property: 'av.sigma'
                gangedProperties: ['av.sigmaV']
                isCurve: true
                minimum: 0
                maximum: 100
            }
        ]
    }
}
