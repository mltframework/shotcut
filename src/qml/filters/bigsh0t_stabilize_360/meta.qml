// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Stabilize")
    keywords: qsTr('spherical smooth', 'search keywords for the 360: Stabilize video filter') + ' 360: stabilize bigsh0t'
    mlt_service: "frei0r.bigsh0t_stabilize_360"
    objectName: "bigsh0t_stabilize_360"
    qml: "ui.qml"
}
