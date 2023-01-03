import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('Mask: Draw') + ' (Glaxnimate)'
    keywords: qsTr('rotoscope matte stencil alpha', 'search keywords for the Mask: Draw video filter') + ' mask: draw'
    mlt_service: 'mask_start'
    objectName: 'maskGlaxnimate'
    qml: 'ui.qml'
    icon: 'icon.webp'
}
