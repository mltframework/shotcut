import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Mask: Chroma Key')
    keywords: qsTr('matte stencil alpha color', 'search keywords for the Mask: Chroma Key video filter') + ' mask: chroma key #rgba #10bit'
    mlt_service: 'mask_start'
    objectName: 'maskChromaKey'
    qml: 'ui.qml'
    icon: 'icon.webp'
    allowMultiple: false
    help: 'https://forum.shotcut.org/t/mask-chroma-key-video-filter/30534/1'
}
