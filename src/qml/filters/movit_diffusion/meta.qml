import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Diffusion")
    keywords: qsTr('blur smooth clean beauty', 'search keywords for the Diffusion video filter') + ' diffusion'
    mlt_service: "movit.diffusion"
    needsGPU: true
    qml: "ui.qml"
}
