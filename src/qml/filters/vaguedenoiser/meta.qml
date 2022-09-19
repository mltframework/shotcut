/*
 * Copyright (c) 2020-2022 Meltytech, LLC
 * Written by Austin Brooks <ab.shotcut@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr('Reduce Noise: Wavelet')
    keywords: qsTr('vague denoise artifact dirt', 'search keywords for the Reduce Noise: Wavelet video filter') + ' reduce noise: wavelet'
    objectName: 'vaguedenoiser'
    mlt_service: 'avfilter.vaguedenoiser'
    qml: 'ui.qml'
    icon: 'icon.webp'
}
