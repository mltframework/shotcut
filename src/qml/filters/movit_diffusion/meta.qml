import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Diffusion")
    keywords: qsTr('blur smooth clean beauty', 'search keywords for the Diffusion video filter') + ' diffusion #gpu'
    mlt_service: "movit.diffusion"
    needsGPU: true
    qml: "ui.qml"
}
