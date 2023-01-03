// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Stabilize")
    keywords: qsTr('spherical smooth deshake', 'search keywords for the 360: Stabilize video filter') + ' 360: stabilize bigsh0t'
    mlt_service: "frei0r.bigsh0t_stabilize_360"
    objectName: "bigsh0t_stabilize_360"
    qml: "ui.qml"
    icon: "icon.webp"
}
