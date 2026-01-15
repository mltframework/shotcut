import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Mask: Apply")
    keywords: qsTr('matte stencil alpha confine composite bounce', 'search keywords for the Mask: Apply video filter') + ' mask: apply #rgba #10bit'
    mlt_service: "mask_apply"
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/mask-apply/12859/1'
}
