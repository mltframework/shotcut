import QtQuick
import org.snapflow.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Mask: Draw') + ' (Glaxnimate)'
    keywords: qsTr('rotoscope matte stencil alpha', 'search keywords for the Mask: Draw video filter') + ' mask: draw #rgba #10bit'
    mlt_service: 'mask_start'
    objectName: 'maskGlaxnimate'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.snapflow.org/t/mask-draw-glaxnimate-video-filter/34408/1'
}
