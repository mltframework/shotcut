import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Mask: Apply")
    keywords: qsTr('matte stencil alpha confine composite bounce', 'search keywords for the Mask: Apply video filter') + ' mask: apply'
    mlt_service: "mask_apply"
    allowMultiple: false
    qml: 'ui.qml'
    icon: 'icon.webp'
}
