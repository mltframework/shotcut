/*
 * Copyright (c) 2018-2022 Meltytech, LLC
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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import QtQuick.Dialogs 1.2
import QtQuick.Window 2.1
import Shotcut.Controls 1.0 as Shotcut
import org.shotcut.qml 1.0 as Shotcut
import QtQml.Models 2.12

Item {
    id: gpsGraphRoot
    width: 350
    height: 800

    property string rectProperty: "rect"
    property rect filterRect: filter.getRect(rectProperty)
    property var allParameters: [
        'time_offset', 'smoothing_value', 'speed_multiplier',
        'graph_data_source', 'graph_type', 'trim_start_p', 'trim_end_p', 'crop_mode_h', 'crop_left_p', 'crop_right_p', 'crop_mode_v', 'crop_bot_p', 'crop_top_p',
        'color_style', 'color.1', 'color.2', 'color.3', 'color.4', 'color.5', 'color.6', 'color.7', 'color.8', 'color.9', 'color.10',
        'show_now_dot', 'now_dot_color', 'show_now_text', 'angle', 'thickness', rectProperty, 'show_grid', 'legend_unit', 'draw_individual_dots',
        'bg_img_path', 'bg_scale_w'
    ]
    property var default_colors: ["#00aaff", "#00e000", "#ffff00", "#ff8c00", "#ff0000"]
    property string color_white: "#ffffff"
    property string default_now_dot: "#00ffffff"
    property string default_rect: '10%/10%:30%x30%'
    property int js_tz_offset: 0

    property bool _disableUpdate: true
    property url settingsOpenPath: 'file:///' + settings.openPath

    Shotcut.File { id: gpsFile }
    Shotcut.File { id: bgFile }
    signal fileOpened(string path)
    onFileOpened: settings.openPath = path

    Component.onCompleted: {
        filter.blockSignals = true

        var resource = filter.get('resource')
        gpsFile.url = resource
        if (filter.isNew) {
            var usedParams
            var _ = undefined //skip params faster

         /* usedParams.push( set_gps_file_params(offset, smooth, speed) )
            usedParams.push( set_graph_data_params(src, type,  startp, endp,  modeh, leftp, rightp,  modev, botp, topp) )
            usedParams.push( set_graph_style_params(style,  nowdot, nowtext,  angle, thick, rect,  grid, legendunit, drawdots) )
            usedParams.push( set_graph_background_params(bgpath, scalew, opacity) )
            usedParams.push( set_graph_colors(cdot, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10) )       */

            //main preset
            usedParams = set_graph_data_params(0,0,  0,100,  0, 0, 100,  0, 0, 100)
            usedParams.push( set_graph_style_params(1, 1, 0, 0, 5,  default_rect, 0, _, 0) )
            usedParams.push( set_graph_colors(color_white, default_colors[0], color_white) )
            filter.savePreset(usedParams, "2D map: full map progress line")

            usedParams = set_graph_data_params(0,1, _,_, 0, 90, 100,  0, 90, 100)
            filter.savePreset(usedParams, "2D map: follow @90%")

            usedParams = set_graph_data_params(0,1, _,_, 0, 95, 100,  0, 95, 100)
            filter.savePreset(usedParams, "2D map: follow @95%")

            usedParams = set_graph_data_params(0,1, _,_, 0, 98, 100,  0, 98, 100)
            filter.savePreset(usedParams, "2D map: follow @98%")

            //1d graphs: speed, altitude, hr
            usedParams = set_graph_data_params(1,0, _,_, 0, 0, 100,  0, 0, 100)
            usedParams.push( set_graph_style_params(1,  1, 1,  0, _, _,  1, 'm', 0) )
            usedParams.push( set_graph_background_params( '!' ) )
            filter.savePreset(usedParams, "1D graph: Altitude")

            usedParams = set_graph_data_params(1,1, _,_, 0, 90, 100,  0, 0, 100)
            usedParams.push( set_graph_style_params(1,  1, 1,  0, _, _,  0, 'm', 0) )
            usedParams.push( set_graph_background_params( '!' ) )
            filter.savePreset(usedParams, "1D graph: Altitude, follow @90%")

            usedParams = set_graph_data_params(2,0, _,_, 0, 0, 100,  0, 0, 100)
            usedParams.push( set_graph_style_params(8,  1, 1,  0, _, _,  1, 'bpm', 0) )
            usedParams.push( set_graph_background_params( '!' ) )
            filter.savePreset(usedParams, "1D graph: HR")

            usedParams = set_graph_data_params(2,1, _,_, 0, 90, 100,  0, 0, 100)
            usedParams.push( set_graph_style_params(8,  1, 1,  0, _, _,  0, 'bpm', 0) )
            usedParams.push( set_graph_background_params( '!' ) )
            filter.savePreset(usedParams, "1D graph: HR, follow @90%")

            usedParams = set_graph_data_params(3,0, _,_, 0, 0, 100,  0, 0, 100)
            usedParams.push( set_graph_style_params(1,  1, 1,  0, _, _,  1, 'km/h', 0) )
            usedParams.push( set_graph_background_params( '!' ) )
            filter.savePreset(usedParams, "1D graph: Speed")

            usedParams = set_graph_data_params(3,1, _,_, 0, 90, 100,  0, 0, 100)
            usedParams.push( set_graph_style_params(1,  1, 1,  0, _, _,  0, 'km/h', 0) )
            usedParams.push( set_graph_background_params( '!' ) )
            filter.savePreset(usedParams, "1D graph: Speed, follow @90%")


            usedParams = set_graph_data_params(0,1, _,_, 0, 99, 100,  0, 99, 100)
            usedParams.push( set_graph_style_params(1, 1, 0, 0, 8, _, _, _, 1) )
            filter.savePreset(usedParams, "Map style: show GPS points as dots @99%")

            usedParams = set_graph_style_params(2, 1, 0, 0, 5,  default_rect, 0, _, 0)
            usedParams.push( set_graph_colors(_, default_colors[0], '#00ffffff') )
            filter.savePreset(usedParams, "Map style: hide future path")

            usedParams = set_graph_style_params(2, 1, 0, 0, 5,  default_rect, 0, _, 0)
            usedParams.push( set_graph_colors(_, default_colors[0], '#40ffffff') )
            filter.savePreset(usedParams, "Map style: 25% opacity thin future path")

            usedParams = set_graph_data_params(3,0,  0,100,  0, 0, 100,  0, 100, 100)
            usedParams.push( set_graph_style_params(1, 1, 0, 0, 5,  '10%/10%:80%x10%', 0, _, 0) )
            usedParams.push( set_graph_colors(color_white, default_colors[0], color_white) )
            usedParams.push( set_graph_background_params("!") )
            filter.savePreset(usedParams, "Simple line progressbar")

            //speedometer example
            usedParams = set_graph_data_params(3,2,  _,_,  0,0,100,  1,0,30)
            usedParams.push( set_graph_style_params(1,  1,1,  0, _, _,  1, 'km/h', _) )
            usedParams.push( set_graph_background_params("!") )
            filter.savePreset(usedParams, "Speedometer: 0-30 km/h")

            usedParams = set_graph_data_params(3,2,  _,_,  0,0,100,  1,0,60)
            usedParams.push( set_graph_style_params(1,  1,1,  0, _, _,  1, 'mi/h', _) )
            usedParams.push( set_graph_background_params("!") )
            filter.savePreset(usedParams, "Speedometer: 0-60 mi/h")

            usedParams = set_graph_data_params(3,2,  _,_,  0,0,100,  0,0,100)
            usedParams.push( set_graph_style_params(1,  1,1,  0, _, _,  0, 'km/h', _) )
            usedParams.push( set_graph_background_params("!") )
            filter.savePreset(usedParams, "Speedometer: min-max km/h")

            //basic text only presets
            usedParams = set_graph_data_params(0,0,  0,100,  0, 0, 0,  0, 0, 0)
            usedParams.push( set_graph_style_params(_, 0, 1, 0, 5,  default_rect, 0, " ", 0) )
            filter.savePreset(usedParams, "Text only: GPS coordonates")
            usedParams = set_graph_data_params(1,0,  0,100,  0, 0, 0,  0, 0, 0)
            usedParams.push( set_graph_style_params(_, 0, 1, 0, 5,  default_rect, 0, 'm', 0) )
            filter.savePreset(usedParams, "Text only: Altitude")
            usedParams = set_graph_data_params(2,0,  0,100,  0, 0, 0,  0, 0, 0)
            usedParams.push( set_graph_style_params(_, 0, 1, 0, 5,  default_rect, 0, 'bpm', 0) )
            filter.savePreset(usedParams, "Text only: Heart rate")
            usedParams = set_graph_data_params(3,0,  0,100,  0, 0, 0,  0, 0, 0)
            usedParams.push( set_graph_style_params(_, 0, 1, 0, 5,  default_rect, 0, 'km/h', 0) )
            filter.savePreset(usedParams, "Text only: Speed")

            usedParams = set_graph_data_params(0,2,  _,_,  0, 0, 100,  0, 0, 100)
            usedParams.push( set_graph_style_params(0, 0, 1, 0, 5,  default_rect, 0, '%', 0) )
            usedParams.push( set_graph_colors(_, "#00000000", "#00000000") )
            filter.savePreset(usedParams, "Text only: Percentage of trimmed track")

            //color presets:
            usedParams = set_graph_style_params(8)
            usedParams.push ( set_graph_colors(_, default_colors[0], default_colors[1], default_colors[2], default_colors[3], default_colors[4]) )
            filter.savePreset(usedParams, "Map colors: gradient of heart rate")
            usedParams = set_graph_style_params(9)
            usedParams.push ( set_graph_colors(_, default_colors[0], default_colors[1], default_colors[2], default_colors[3], default_colors[4]) )
            filter.savePreset(usedParams, "Map colors: gradient of speed")
            usedParams = set_graph_colors(_, default_colors[0], default_colors[1], default_colors[2], default_colors[3], default_colors[4])
            filter.savePreset(usedParams, "Colors: default (5)")
            usedParams = set_graph_colors(_, default_colors[4], default_colors[3], default_colors[2], default_colors[1], default_colors[0])
            filter.savePreset(usedParams, "Colors: inversed default (5)")

            usedParams = reset_all_params()
            filter.savePreset(usedParams)
        }

        //get current timezone
        //TODO: not sure why this doesn't include daylight saving if active, remove completely?
        var today = new Date()
        js_tz_offset = today.getTimezoneOffset()*60
        filter.blockSignals = false

        setControls()
    }


    //gps funcs
    FileDialog {
        id: fileDialog
        title: "Select GPS file"
        folder: settingsOpenPath
        modality: application.dialogModality
        selectMultiple: false
        selectFolder: false
        nameFilters: ['Supported files (*.gpx *.tcx)', 'GPS Exchange Format (*.gpx)', 'Training Center XML (*.tcx)']
        onAccepted: {
            gpsFile.url = fileDialog.fileUrl
            gpsGraphRoot.fileOpened(gpsFile.path)
            fileLabel.text = gpsFile.fileName
            fileLabel.color = activePalette.text
            fileLabelTip.text = gpsFile.filePath
            console.log("url= " + gpsFile.url)
            filter.set('resource', gpsFile.url)
            filter.set('gps_start_text', '')
            gpsFinishParseTimer.restart()
        }
    }

    //timer to update UI after gps file is processed; max: 10x250ms
    Timer {
        id: gpsFinishParseTimer
        interval: 250
        repeat: true
        triggeredOnStart: false
        property int calls: 0
        onTriggered: {
            if (filter.get('gps_start_text') == '') {
                calls += 1;
                if (calls > 10) {
                    gpsFinishParseTimer.stop();
                    calls = 0;
                    setControls();
                }
            }
            else {
                gpsFinishParseTimer.stop();
                calls = 0;
                setControls();
            }
        }
    }

    //this function combines the text values from sign combobox * days/hours/mins/sec TextFields into an int
    function recompute_time_offset() {
        var offset_sec = parseInt(Number(offset_days.text), 10)*24*60*60 +
                         parseInt(Number(offset_hours.text), 10)*60*60 +
                         parseInt(Number(offset_mins.text), 10)*60 +
                         parseInt(Number(offset_secs.text), 10);
        offset_sec *= parseInt(filter.get('majoroffset_sign'), 10);
        filter.set('time_offset', Number(offset_sec).toFixed(0))
    }

    //transforms a wheel-up/down event into the correct offset direction
    function wheel_offset(val) {
        var offset = Number(filter.get("time_offset"));
        if (offset < 0)
            val *= -1;
        if (offset === 0 && val < 0) return; //fix unnatural behaviour when subtracting at offset 0
        set_sec_offset_to_textfields( offset + val );
    }

    //splits (and fills) seconds into days/hours/mins/secs textfields
    function set_sec_offset_to_textfields(secs) {
        if (secs === '')
            return;

        if (secs < 0) {
            combo_majoroffset_sign.currentIndex = 1
            filter.set('majoroffset_sign', -1)
        }
        else {
            combo_majoroffset_sign.currentIndex = 0
            filter.set('majoroffset_sign', 1)
        }

        offset_days.text = parseInt( Math.abs(secs)/(24*60*60) , 10 )
        offset_hours.text = parseInt( Math.abs(secs)/(60*60)%24 , 10 )
        offset_mins.text = parseInt( Math.abs(secs)/60%60 , 10 )
        offset_secs.text = parseInt( Math.abs(secs)%60 , 10 )

        //toFixed(0) avoids scientific notation!
        filter.set('time_offset', Number(secs).toFixed(0))
    }

    //graph rect
    function setFilter() {
        _disableUpdate = true
        var x = rectX.value
        var y = rectY.value
        var w = rectW.value
        var h = rectH.value
        if (x !== filterRect.x ||
            y !== filterRect.y ||
            w !== filterRect.width ||
            h !== filterRect.height) {
            filterRect.x = x
            filterRect.y = y
            filterRect.width = w
            filterRect.height = h
            filter.set(rectProperty, filterRect)
        }
        _disableUpdate = false
    }

    function setControls() {
        _disableUpdate = true

        //keep in UI order for sanity!

        //gps sync data
        if (gpsFile.exists()) {
            fileLabel.text = gpsFile.fileName
            fileLabelTip.text = gpsFile.filePath + "\nGPS start time: " + filter.get('gps_start_text') + "\nVideo start time: " + filter.get('video_start_text')
        } else {
            fileLabel.text = qsTr("No File Loaded.")
            fileLabel.color = 'red'
            fileLabelTip.text = qsTr('No GPS file loaded.\nClick "Open" to load a file.')
        }
        set_sec_offset_to_textfields(filter.get('time_offset'));
        combo_smoothing.currentIndex = combo_smoothing.get_smooth_index_from_val(filter.get('smoothing_value'));
        speed_multiplier.value = filter.get('speed_multiplier');

        //graph data
        combo_data_source.currentIndex = filter.get("graph_data_source")
        combo_graph_type.currentIndex = filter.get("graph_type")
        spin_start.value = crop_start_end_slider.first.value = filter.getDouble('trim_start_p')
        spin_end.value = crop_start_end_slider.second.value = filter.getDouble('trim_end_p')
        combo_cropmode_h.currentIndex = filter.get("crop_mode_h")
        combo_cropmode_v.currentIndex = filter.get("crop_mode_v")
        spin_bot.value = crop_top_bot_slider.first.value = filter.getDouble('crop_bot_p')
        spin_top.value = crop_top_bot_slider.second.value = filter.getDouble('crop_top_p')
        spin_left.value = crop_left_right_slider.first.value = filter.getDouble('crop_left_p')
        spin_right.value = crop_left_right_slider.second.value = filter.getDouble('crop_right_p')

        //graph design
        combo_color_style.currentIndex = filter.get("color_style")
        //todo: fix gradient number of colors after project reopen
        colGradient.colors = filter.getGradient('color') //.splice(0, combo_color_style.get_combo_gradient_nr_colors())
        checkbox_now_dot.checked = (filter.get("show_now_dot") == 1)
        checkbox_now_text.checked = (filter.get("show_now_text") == 1)
        now_dot_color.value = filter.get('now_dot_color')
        graph_rotation.value = filter.get("angle")
        thicknessSlider.value = filter.getDouble('thickness')
        checkbox_grid.checked = (filter.get("show_grid") == 1)
        legend_unit.text = filter.get("legend_unit")
        rectX.value = filterRect.x
        rectY.value = filterRect.y
        rectW.value = filterRect.width
        rectH.value = filterRect.height
        //advanced options
//        checkbox_draw_individual_dots.checked = (filter.get("draw_individual_dots") == 1)
        bg_img_path.text = filter.get("bg_img_path")
        slider_scaleW.value = filter.getDouble('bg_scale_w')

        _disableUpdate = false
    }

    function reset_legend_unit() {
        if (combo_data_source.currentIndex === 1)
            legend_unit.text = "m";
        else if (combo_data_source.currentIndex === 2)
            legend_unit.text = "bpm";
        else if (combo_data_source.currentIndex === 3)
            legend_unit.text = "km/h";
        else
            legend_unit.text = "";
    }

    FileDialog {
        id: selectBgImage
        title: "Select background image"
        folder: settingsOpenPath
        modality: application.dialogModality
        selectMultiple: false
        selectFolder: false
        nameFilters: ['Image files (*.jpg *.jpeg *.png)', 'JPG (*.jpg *.jpeg)', 'PNG (*.png)', 'All files (*)']
        onAccepted: {
            bgFile.url = selectBgImage.fileUrl
            bg_img_path.text = bgFile.filePath
            gpsGraphRoot.fileOpened(bgFile.path)
            filter.set("bg_img_path", bgFile.filePath)
        }
    }


    GridLayout {
        id: mainGrid
        columns: 2
        anchors.fill: parent
        anchors.margins: 8

        Shotcut.Button {
            id: openButton
            text: qsTr('Open file')
            Layout.alignment: Qt.AlignRight
            onClicked: {
                fileDialog.open()
            }
        }
        Label {
            id: fileLabel
            Layout.fillWidth: true
            Shotcut.HoverTip { id: fileLabelTip }
        }

        Label {
            Layout.topMargin: 10
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            Layout.topMargin: 10
            id: preset
            parameters: allParameters
            onBeforePresetLoaded: {
                filter.resetProperty("legend_unit")
            }

            onPresetSelected: {
                filterRect = filter.getRect(rectProperty)
                setControls()
                reset_legend_unit()
            }
        }

        Label {
            topPadding: 10
            text: qsTr('<b>GPS options</b>')
            Layout.columnSpan: 2
        }

        Label {
            id: gps_sync_major
            text: qsTr('GPS offset')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('This is added to video time to sync with gps time.') }
        }
        RowLayout {
            Shotcut.ComboBox {
                id: combo_majoroffset_sign
                implicitWidth: 39
                model: ListModel {
                    id: sign_val
                    ListElement { text: '+'; value: 1}
                    ListElement { text: '-'; value: -1}
                }
                currentIndex: 0
                textRole: 'text'
                Shotcut.HoverTip { text: qsTr('+ : Adds time to video (use if GPS is ahead).\n - : Subtracts time from video (use if video is ahead).') }
                onActivated: {
                    if (_disableUpdate) return
                    filter.set('majoroffset_sign', sign_val.get(currentIndex).value)
                    recompute_time_offset()
                }
            }
//            Label {
//                text: qsTr('days:')
//                Layout.alignment: Qt.AlignRight
//            }
            TextField {
                id: offset_days
                text: '0'
                horizontalAlignment: TextInput.AlignRight
                validator: IntValidator {bottom: 0; top: 36600;}
                implicitWidth: 30
                MouseArea {
                    anchors.fill: parent
                    onWheel: wheel_offset( wheel.angleDelta.y>0 ? 86400 : -86400 )
                    onClicked: offset_days.forceActiveFocus()
                }
                onFocusChanged: if (focus) selectAll()
                onEditingFinished: recompute_time_offset()
                Shotcut.HoverTip { text: qsTr('Number of days to add/subtract to video time to sync them.') }
            }
            Label {
                text: qsTr(':')
                Layout.alignment: Qt.AlignRight
            }
            TextField {
                id: offset_hours
                text: '0'
                horizontalAlignment: TextInput.AlignRight
                validator: IntValidator {bottom: 0; top: 59;}
                implicitWidth: 30
                MouseArea {
                    anchors.fill: parent
                    onWheel: wheel_offset( wheel.angleDelta.y>0 ? 3600 : -3600 )
                    onClicked: { offset_hours.forceActiveFocus() }
                }
                onFocusChanged: if (focus) selectAll()
                onEditingFinished: recompute_time_offset();
                Shotcut.HoverTip { text: qsTr('Number of hours to add/subtract to video time to sync them.') }
            }
            Label {
                text: qsTr(':')
                Layout.alignment: Qt.AlignRight
            }
            TextField {
                id: offset_mins
                text: '0'
                horizontalAlignment: TextInput.AlignRight
                validator: IntValidator {bottom: 0; top: 59;}
                implicitWidth: 30
                MouseArea {
                    anchors.fill: parent
                    onWheel: wheel_offset( wheel.angleDelta.y>0 ? 60 : -60 )
                    onClicked: { offset_mins.forceActiveFocus() }
                }
                onFocusChanged: if (focus) selectAll()
                onEditingFinished: recompute_time_offset()
                Shotcut.HoverTip { text: qsTr('Number of minutes to add/subtract to video time to sync them.') }
            }
            Label {
                text: qsTr(':')
                Layout.alignment: Qt.AlignRight
            }
            TextField {
                id: offset_secs
                text: '0'
                horizontalAlignment: TextInput.AlignRight
                validator: IntValidator {bottom: 0; top: 59;}
                implicitWidth: 30
                MouseArea {
                    anchors.fill: parent
                    onWheel: wheel_offset( wheel.angleDelta.y>0 ? 1 : -1 )
                    onClicked: { offset_secs.forceActiveFocus() }
                }
                onFocusChanged: if (focus) selectAll()
                onEditingFinished: recompute_time_offset()
                Shotcut.HoverTip { text: qsTr('Number of seconds to add/subtract to video time to sync them.\nTip: you can use mousewheel to change values.') }
            }

            //buttons:
            Shotcut.Button {
                icon.name: 'media-skip-backward'
                Shotcut.HoverTip { text: qsTr('Sync start of GPS to start of video file.\nTip: use this if you started GPS and video recording at the same time.') }
                implicitWidth: 20
                implicitHeight: 20
                onClicked: { set_sec_offset_to_textfields(filter.get('auto_gps_offset_start')) }
            }
            Shotcut.Button {
                icon.name: 'document-open-recent'
                Shotcut.HoverTip { text: qsTr('Remove timezone (%1 seconds) time from video file (convert to UTC).\nTip: use this if your video camera doesn\'t have timezone settings as it will set local time as UTC.'.arg(js_tz_offset) ) }
                implicitWidth: 20
                implicitHeight: 20
                onClicked: { set_sec_offset_to_textfields( parseInt(Number(filter.get('time_offset'))) + parseInt(Number(js_tz_offset)) ) }
            }
            Shotcut.Button {
                icon.name: 'format-indent-less'
                Shotcut.HoverTip { text: qsTr('Fix video start time: if file time is actually end time, press this button to subtract file length (%1 seconds) from GPS offset.'.arg(parseInt(producer.length / profile.fps)) ) }
                implicitWidth: 20
                implicitHeight: 20
                onClicked: { set_sec_offset_to_textfields( parseInt(Number(filter.get('time_offset')) - producer.length / profile.fps) ); }
            }
            Shotcut.Button {
                icon.name: 'media-playback-pause'
                Shotcut.HoverTip { text: qsTr('Sync start of GPS to current video time.\nTip: use this if you recorded the moment of the first GPS fix.') }
                implicitWidth: 20
                implicitHeight: 20
                onClicked: { set_sec_offset_to_textfields(filter.get('auto_gps_offset_now')) }
            }
            Shotcut.UndoButton {
                onClicked: set_sec_offset_to_textfields( 0 )
            }
        }

        Label {
            text: qsTr('GPS smoothing')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Average nearby GPS points to smooth out errors.') }
        }
        RowLayout {
            Shotcut.ComboBox {
                id: combo_smoothing
                implicitWidth: 300
                model: ListModel {
                    id: smooth_val_list
                    ListElement { text: '1 point (no smoothing)'; value: 1}
                    ListElement { text: '3 points'; value: 3}
                    ListElement { text: '5 points'; value: 5}
                    ListElement { text: '7 points'; value: 7}
                    ListElement { text: '11 points'; value: 11}
                    ListElement { text: '15 points'; value: 15}
                    ListElement { text: '31 points'; value: 31}
                    ListElement { text: '63 points'; value: 63}
                    ListElement { text: '127 points'; value: 127}
                }
                textRole: 'text'
                currentIndex: 2
                onActivated: {
                    if (_disableUpdate) return
                    filter.set('smoothing_value', smooth_val_list.get(currentIndex).value)
                }
                function get_smooth_index_from_val(val) {
                    for (var i=0; i<smooth_val_list.count; i++) {
                        if (smooth_val_list.get(i).value == val)
                            return i
                    }
                    console.log("get_smooth_index_from_val: no match for smooth val= "+val)
                    return 2 //default
                }
            }
            Shotcut.UndoButton {
                onClicked: {
                    combo_smoothing.currentIndex = 2
                    filter.set('smoothing_value', 5)
                }
            }
        }

        Label {
            text: qsTr('Video speed')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('If the current video is sped up (timelapse) or slowed down use this field to set the speed.') }
        }
        RowLayout {
            Shotcut.DoubleSpinBox {
                id: speed_multiplier
                value: 1
                horizontalAlignment: Qt.AlignRight
                Shotcut.HoverTip { text: qsTr('Fractional times are also allowed (0.25 = 4x slow motion, 5 = 5x timelapse).') }
                Layout.minimumWidth: 80
                from: 0
                to: 10000
                decimals: 2
                stepSize: 1
                suffix: 'x'
                onValueChanged: {
                    if (_disableUpdate) return;
                    filter.set('speed_multiplier', speed_multiplier.value);
                }
            }
            Shotcut.UndoButton {
                onClicked: {
                    filter.set('speed_multiplier', 1)
                    speed_multiplier.value = 1;
                }
            }
        }


        Label {
            topPadding: 10
            text: qsTr('<b>Graph data</b>')
            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('Data source')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Choose which data type is used for graph drawing.') }
        }
        RowLayout {
            Shotcut.ComboBox {
                implicitWidth: 300
                id: combo_data_source
                model:
                    [   qsTr('Location (2D map)'),
                        qsTr('Altitude'),
                        qsTr('Heart rate'),
                        qsTr('Speed'),
                    ]
                onActivated: {
                    if (_disableUpdate) return
                    filter.set('graph_data_source', currentIndex)
                    reset_legend_unit()
                    setControls()
                }
            }
        }

        Label {
            text: qsTr('Graph type')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Graph types can add advanced interactions.') }
        }
        RowLayout {
            Shotcut.ComboBox {
                implicitWidth: 300
                id: combo_graph_type
                Shotcut.HoverTip { text: qsTr('Standard = just a static map.\nFollow dot = centers on the current location.\nSpeedometer = draws a simple speedometer.') }
                model:
                    [   qsTr('Standard'),
                        qsTr('Follow dot (cropped)'),
                        qsTr('Speedometer')
                    ]
                onActivated: {
                    if (_disableUpdate) return
                    filter.set('graph_type', currentIndex)
                    setControls()
                }
            }
        }

        Label {
            text: qsTr('Trim time')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Hides part of the graph at begining or end.\nThis does not recompute min/max for any field.') }
        }
        GridLayout {
            columns: 2
            RowLayout {
                Shotcut.DoubleSpinBox {
                    id: spin_start
                    value: 0
                    horizontalAlignment: Qt.AlignRight
                    Shotcut.HoverTip { text: qsTr('Hides part of the begining of the graph.') }
                    Layout.minimumWidth: 80
                    from: 0
                    to: 100
                    decimals: 1
                    stepSize: 1 / Math.pow(10, decimals)
                    suffix: '%'
                    onValueChanged: {
                        if (_disableUpdate) return
                        filter.set("trim_start_p", value)
                        setControls()
                    }
                }
                RangeSlider {
                    id: crop_start_end_slider
                    from: 0
                    to: 100
                    first.value: 0
                    second.value: 100
                    first.onMoved: {
                        if (_disableUpdate) return
                        filter.set("trim_start_p", first.value)
                        setControls()
                    }
                    second.onMoved: {
                        if (_disableUpdate) return
                        filter.set("trim_end_p", second.value)
                        setControls()
                    }
                }
                Shotcut.DoubleSpinBox {
                    id: spin_end
                    value: 100
                    horizontalAlignment: Qt.AlignRight
                    Shotcut.HoverTip { text: qsTr('Hides part of the end of the graph.') }
                    Layout.minimumWidth: 80
                    from: 0
                    to: 100
                    decimals: 1
                    stepSize: 1 / Math.pow(10, decimals)
                    suffix: '%'
                    onValueChanged: {
                        if (_disableUpdate) return
                        filter.set("trim_end_p", value)
                        setControls()
                    }
                }
            }
            Shotcut.UndoButton {
                onClicked: {
                    if (_disableUpdate) return
                    filter.set("trim_start_p", 0)
                    filter.set("trim_end_p", 100)
                    setControls()
                }
            }
        }

        Label {
            text: qsTr('Crop horizontal')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Zooms in on the graph on the horizontal axis (longitude if map, time if simple graph).\nThe number is either a percentage or a numeric value interpreted as the legend type.\nThis field is not applicable for Speedometer type.') }
        }
        RowLayout {
            Shotcut.DoubleSpinBox {
                id: spin_left
                horizontalAlignment: Qt.AlignRight
                Shotcut.HoverTip { text: qsTr('Crops the graph from the left side.') }
                Layout.minimumWidth: 80
                from: -999999
                to: 999999
                decimals: 1
                stepSize: 1 / Math.pow(10, decimals)
                onValueChanged: {
                    if (_disableUpdate) return
                    filter.set("crop_left_p", value)
                    setControls()
                }
            }
            RangeSlider {
                id: crop_left_right_slider
                from: -10
                to: 110
                first.value: 0
                second.value: 100
                first.onMoved: {
                    if (_disableUpdate) return
                    filter.set("crop_left_p", first.value)
                    setControls()
                }
                second.onMoved: {
                    if (_disableUpdate) return
                    filter.set("crop_right_p", second.value)
                    setControls()
                }
            }
            Shotcut.DoubleSpinBox {
                id: spin_right
                value: 100
                horizontalAlignment: Qt.AlignRight
                Shotcut.HoverTip { text: qsTr('Crops the graph from the right side. This value is ignored if mode is Follow dot.') }
                Layout.minimumWidth: 80
                from: -999999
                to: 999999
                decimals: 1
                stepSize: 1 / Math.pow(10, decimals)
                onValueChanged: {
                    if (_disableUpdate) return
                    filter.set("crop_right_p", value)
                    setControls()
                }
            }
            Shotcut.ComboBox {
                id: combo_cropmode_h
                Shotcut.HoverTip { text: qsTr('The crop values are interpreted as a percentage of total or as an absolute value (in legend unit).') }
                implicitWidth: 40
                model:
                    [   qsTr('%'),
                        qsTr('value')
                    ]
                currentIndex: 0
                onActivated: {
                    if (_disableUpdate) return
                    filter.set('crop_mode_h', currentIndex)
                    setControls()
                }
                Shotcut.HoverTip { text: qsTr('Input for horizontal crops can be a percentage or an absolute value.') }
            }
            Shotcut.UndoButton {
                onClicked: {
                    if (_disableUpdate) return
                    filter.set("crop_left_p", 0)
                    filter.set("crop_right_p", 100)
                    filter.set('crop_mode_h', 0)
                    setControls()
                }
            }
        }

        Label {
            text: qsTr('Crop vertical')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Zooms in on the graph on the vertical axis (latitude if map, value if simple graph).\nThe number is either a percentage or a numeric value interpreted as the legend type.\nThis field affects min/max values on the Speedometer type.') }
        }
        RowLayout {
            Shotcut.DoubleSpinBox {
                id: spin_bot
                value: 0
                horizontalAlignment: Qt.AlignRight
                Shotcut.HoverTip { text: qsTr('Crops the graph from the bottom side.') }
                Layout.minimumWidth: 80
                from: -999999
                to: 999999
                decimals: 1
                stepSize: 1 / Math.pow(10, decimals)
                onValueChanged: {
                    if (_disableUpdate) return
                    filter.set("crop_bot_p", value)
                    setControls()
                }
            }
            RangeSlider {
                id: crop_top_bot_slider
                from: -10
                to: 110
                first.value: 0
                second.value: 100
                first.onMoved: {
                    if (_disableUpdate) return
                    filter.set("crop_bot_p", first.value)
                    setControls()
                }
                second.onMoved: {
                    if (_disableUpdate) return
                    filter.set("crop_top_p", second.value)
                    setControls()
                }
            }
            Shotcut.DoubleSpinBox {
                id: spin_top
                value: 100
                horizontalAlignment: Qt.AlignRight
                Shotcut.HoverTip { text: qsTr('Crops the graph from the top side. This value is ignored if mode is Follow dot.') }
                Layout.minimumWidth: 80
                from: -999999
                to: 999999
                decimals: 1
                stepSize: 1 / Math.pow(10, decimals)
                onValueChanged: {
                    if (_disableUpdate) return
                    filter.set("crop_top_p", value)
                    setControls()
                }
            }
            Shotcut.ComboBox {
                id: combo_cropmode_v
                Shotcut.HoverTip { text: qsTr('The crop values are interpreted as a percentage of total or as an absolute value (in legend unit).') }
                implicitWidth: 40
                model:
                    [   qsTr('%'),
                        qsTr('value')
                    ]
                currentIndex: 0
                onActivated: {
                    if (_disableUpdate) return
                    filter.set('crop_mode_v', currentIndex)
                    setControls()
                }
                Shotcut.HoverTip { text: qsTr('Input for vertical crops can be a percentage or an absolute value.') }
            }
            Shotcut.UndoButton {
                onClicked: {
                    if (_disableUpdate) return
                    filter.set("crop_bot_p", 0)
                    filter.set("crop_top_p", 100)
                    filter.set('crop_mode_v', 0)
                    setControls()
                }
            }
        }

        Label {
            topPadding: 10
            text: qsTr('<b>Graph design</b>')
            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('Color style')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Choose how you want to color the graph line.') }
        }
        RowLayout {
            Shotcut.ComboBox {
                implicitWidth: 300
                id: combo_color_style
                model:
                    [
                        qsTr('One color'),              //0
                        qsTr('Two colors'),
                        qsTr('Solid past, thin future'),
                        qsTr('Solid future, thin past'),
                        qsTr('Vertical gradient'),      //4
                        qsTr('Horizontal gradient'),
                        qsTr('Color by duration'),      //6
                        qsTr('Color by altitude'),
                        qsTr('Color by heart rate'),
                        qsTr('Color by speed')
                    ]
                currentIndex: 1
                onActivated: {
                    if (_disableUpdate) return
                    filter.set('color_style', currentIndex)
                    colGradient.set_defcolors_in_gradient_control()
                    setControls()
                }
                function get_combo_gradient_nr_colors() {
                    if (combo_color_style.currentIndex === 0)
                        return 1
                    else if (combo_color_style.currentIndex <=3)
                        return 2
                    else
                        return filter.getGradient('color').length
                }
            }
        }

        Label {
            text: qsTr('Color')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Color by Altitude/HR only work if there are recorded values in the gps file.\nFor speedometer type, only first 2 colors are used.') }
        }
        RowLayout {
            Shotcut.GradientControl {
                id: colGradient
                onGradientChanged: {
                    if (_disableUpdate) return
                    filter.setGradient('color', colors)
                }
                Shotcut.UndoButton {
                    onClicked: {
                        if (_disableUpdate) return
                        colGradient.set_defcolors_in_gradient_control()
                        setControls()
                    }
                }
                function set_defcolors_in_gradient_control() {
                    filter.setGradient('color', [])
                    if (combo_color_style.currentIndex === 0) {
                        colGradient.colors = default_colors.slice(0, 1)
                    }
                    else if (combo_color_style.currentIndex <=3) {
                        colGradient.colors = default_colors.slice(0, 2)
                        colGradient.colors[1] = color_white
                    }
                    else {
                        colGradient.colors = default_colors.slice(0, 5)
                    }
                    filter.setGradient('color', colGradient.colors)
                }
            }
        }


        Label {
            text: qsTr('Now dot')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Draw a dot showing current position on the graph.\nFor speedometer type, this is the needle.') }
        }
        RowLayout {
            CheckBox {
                id: checkbox_now_dot
                checked: true
                leftPadding: 0
                onClicked: filter.set('show_now_dot', checked ? 1 : 0)
            }
            Label {
                text: qsTr('Color')
                Layout.alignment: Qt.AlignRight
                Shotcut.HoverTip { text: qsTr('Set the color of the inside of the now_dot (or needle).') }
            }
            Shotcut.ColorPicker {
                id: now_dot_color
                eyedropper: true
                alpha: true
                onValueChanged: {
                    if (_disableUpdate) return
                    filter.set('now_dot_color', value)
                }
            }
            Shotcut.UndoButton {
                onClicked: {
                    now_dot_color.value = default_now_dot
                    filter.set('now_dot_color', default_now_dot)
                }
            }
            Label {
                text: qsTr('Now text')
                leftPadding: 5
                Layout.alignment: Qt.AlignRight
                Shotcut.HoverTip { text: qsTr('Draw a large white text showing the current value.\nThe legend unit (if present) will be appended at the end.') }
            }
            CheckBox {
                id: checkbox_now_text
                leftPadding: 0
                checked: true
                onClicked: filter.set('show_now_text', checked ? 1 : 0)
            }
        }

        Label {
            text: qsTr('Rotation')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Rotate the entire graph. Speedometer also rotates internal text.') }
        }
        RowLayout {
            Shotcut.SliderSpinner {
                id: graph_rotation
                minimumValue: -360
                maximumValue: 360
                decimals: 1
                suffix: qsTr(' °', 'degrees')
                onValueChanged: {
                    if (_disableUpdate) return
                    filter.set('angle', graph_rotation.value)
                }
            }
            Shotcut.UndoButton {
                onClicked: {
                    if (_disableUpdate) return
                    graph_rotation.value = 0
                    filter.set('angle', 0)
                }
            }
        }

        Label {
            text: qsTr('Thickness')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Set the thickness of the graph line. Does not affect speedometer.') }
        }
        RowLayout {
            Shotcut.SliderSpinner {
                id: thicknessSlider
                minimumValue: 1
                maximumValue: 10
                decimals: 0
                suffix: ' px'
                onValueChanged: {
                    if (_disableUpdate) return
                    filter.set("thickness", value)
                }
            }
            Shotcut.UndoButton {
                onClicked: {
                    if (_disableUpdate) return
                    thicknessSlider.value = 5
                    filter.set("thickness", 5)
                }
            }
        }

        Label {
            text: qsTr('Draw legend')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Draw 5 horizontal white lines with individual values for graph readability. 2D map also draws vertical (longitude) lines.\nFor speedometer this draws text for divisions.') }
        }
        RowLayout {
            CheckBox {
                id: checkbox_grid
                leftPadding: 0
                onClicked: filter.set('show_grid', checked ? 1 : 0)
            }
            Label {
                text: qsTr('Unit')
                Layout.alignment: Qt.AlignRight
                Shotcut.HoverTip { text: qsTr('This will be used in legend text if active and in absolute value math.') }
            }
            TextField {
                id: legend_unit
                horizontalAlignment: TextInput.AlignRight
                implicitWidth: 80
                Shotcut.HoverTip { text: qsTr('Defaults are km/h (speed) and meters (altitude).\n Available options: km/h, mi/h, nm/h (kn), m/s, ft/s.') }
                onEditingFinished: filter.set('legend_unit', legend_unit.text);
            }
            Shotcut.UndoButton {
                onClicked: {
                    if (_disableUpdate) return
                    reset_legend_unit();
                    filter.set('legend_unit', legend_unit.text);
                }
            }
        }

//        Label {
//            Layout.alignment: Qt.AlignRight
//            text: qsTr('Draw dots only')
//            Shotcut.HoverTip { text: qsTr('Change graph draw style to only show individual points instead of lines. Must be very zoomed in to see them.') }
//        }
//        CheckBox {
//            id: checkbox_draw_individual_dots
//            leftPadding: 0
//            onClicked: filter.set('draw_individual_dots', checked ? 1 : 0)
//        }


        Label {
            text: qsTr('Position')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Shotcut.DoubleSpinBox {
                id: rectX
                value: filterRect.x
                Layout.minimumWidth: 80
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: {
                    if (_disableUpdate) return
                    setFilter()
                }
            }
            Label { text: ','; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            Shotcut.DoubleSpinBox {
                id: rectY
                value: filterRect.y
                Layout.minimumWidth: 80
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: {
                    if (_disableUpdate) return
                    setFilter()
                }
            }
            Shotcut.UndoButton {
                onClicked: {
                    rectX.value = profile.width * 0.10
                    rectY.value = profile.height * 0.10
                    setFilter()
                }
            }
        }

        Label {
            text: qsTr('Size')
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Shotcut.DoubleSpinBox {
                id: rectW
                value: filterRect.width
                Layout.minimumWidth: 80
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: {
                    if (_disableUpdate) return
                    setFilter()
                }
            }
            Label { text: 'x'; Layout.minimumWidth: 20; horizontalAlignment: Qt.AlignHCenter }
            Shotcut.DoubleSpinBox {
                id: rectH
                value: filterRect.height
                Layout.minimumWidth: 80
                horizontalAlignment: Qt.AlignRight
                decimals: 0
                stepSize: 1
                from: -999999999
                to: 999999999
                onValueModified: {
                    if (_disableUpdate) return
                    setFilter()
                }
            }
            Shotcut.UndoButton {
                Shotcut.HoverTip { text: qsTr('Sets the height to the correct map aspect ratio or 1:1.') }
                onClicked: {
                    //for map, try to compute height from width using map_aspect_ratio and the crop values
                    if (combo_data_source.currentIndex === 0) {
                        var width_p = (spin_right.value - spin_left.value)/100.0;
                        var height_p = (spin_top.value - spin_bot.value)/100.0;
                        rectH.value = 1.0 * ((rectW.value * height_p) / filter.getDouble('map_original_aspect_ratio')) / width_p;
                    }
                    else {
                        rectH.value = rectW.value;
                    }
                    //if it's outside viewport just make it inside, ignore map or crops
                    if (rectW.value + rectX.value > profile.width)
                        rectW.value = profile.width - rectX.value
                    if (rectH.value + rectY.value > profile.height)
                        rectH.value = profile.height - rectY.value
                    setFilter();
                }
            }
        }


        Label {
            topPadding: 10
            text: qsTr('<b>Background options</b>')
            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('Image path')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Choose an image to overlay behind the graph. Tip: you can use an actual map image to make the GPS track more interesting.') }
        }
        RowLayout {
            TextField {
                id: bg_img_path
                Layout.alignment: Qt.AlignLeft
                implicitWidth: 300
                selectByMouse: true
                onEditingFinished: {
                    if (bg_img_path.text[0] !== '!')
                        filter.set('bg_img_path', text)
                }
            }
            Button {
                icon.name: 'dialog-information'
                Shotcut.HoverTip { text: qsTr('Get the center coordonate of GPS map. This does not change with trim or crop.\nTIP:OpenStreetMap website can save the current standard map centered on searched location (but only at screen resolution).\nGoogle Earth for desktop can center on a coordonate and save a 4K image of it. Disable the Terrain layer for best results.') }
                implicitWidth: 20
                implicitHeight: 20
                onClicked: bg_img_path.text = "! " + qsTr("GPS file center is: ") + filter.get("map_coords_hint")
            }
            Button {
                icon.name: 'document-open'
                Shotcut.HoverTip { text: qsTr('Browse for an image file to be assigned as graph background.') }
                implicitWidth: 20
                implicitHeight: 20
                onClicked: selectBgImage.open()
            }
            Shotcut.UndoButton {
                onClicked: {
                    bg_img_path.text = "";
                    filter.set("bg_img_path", "")
                }
            }
        }

        Label {
            text: qsTr('Scale')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip { text: qsTr('Increase or decrease the size of the background image.\nValues smaller than 1 will zoom into image.') }
        }
        RowLayout {
            Shotcut.SliderSpinner {
                id: slider_scaleW
                minimumValue: 0
                maximumValue: 2
                decimals: 3
                stepSize: 1 / Math.pow(10, decimals)
                onValueChanged: {
                    if (_disableUpdate) return
                    filter.set('bg_scale_w', value)
                }
            }
            Shotcut.UndoButton {
                onClicked: {
                    slider_scaleW.value = 1
                    filter.set('bg_scale_w', 1)
                }
            }
        }

//        Label {
//            topPadding: 10
//            text: qsTr('<b>Advanced options</b>')
//            Layout.columnSpan: 2
//        }

        Item {
            Layout.fillHeight: true
        }
    }

    Connections {
        target: filter
        onChanged: {
            var newValue = filter.getRect(rectProperty)
            if (filterRect !== newValue) {
                filterRect = newValue
//                filter.set(rectProperty, newValue)
//                setFilter()
                setControls()
            }
        }
    }

    //just filter.set(param, val) but returns the param name so I don't have to manually compose a used_params for presets
    function filterset_getparams (param, val)
    {
        filter.set(param, val)
        return param
    }
    function reset_all_params()
    {
        var used_params = []
        used_params.push(set_gps_file_params(0, 5, 1))
        used_params.push(set_graph_data_params(0, 0,  0, 100,  0, 0, 100,  0, 0, 100,))
        used_params.push(set_graph_style_params(1, 1, 0, 0, 5, default_rect, 0, '', 0))
        used_params.push(set_graph_colors(default_now_dot, default_colors[0], color_white, default_colors[2], default_colors[3], default_colors[4], default_colors[5]))
        used_params.push(set_graph_background_params("!", 1, 1))
        return used_params
    }
    function set_gps_file_params(offset, smooth, speed)
    {
        var used_params = []
        if (offset !== undefined) used_params.push(filterset_getparams('time_offset', offset))
        if (smooth !== undefined) used_params.push(filterset_getparams('smoothing_value', smooth))
        if (speed !== undefined) used_params.push(filterset_getparams('speed_multiplier', speed))
        return used_params
    }
    function set_graph_data_params(src, type, startp, endp, modeh, leftp, rightp, modev, botp, topp)
    {
        var used_params = []
        if (src !== undefined) used_params.push(filterset_getparams('graph_data_source', src))
        if (type !== undefined) used_params.push(filterset_getparams('graph_type', type))
        if (startp !== undefined) used_params.push(filterset_getparams('trim_start_p', startp))
        if (endp !== undefined) used_params.push(filterset_getparams('trim_end_p', endp))
        if (modeh !== undefined) used_params.push(filterset_getparams('crop_mode_h', modeh))
        if (leftp !== undefined) used_params.push(filterset_getparams('crop_left_p', leftp))
        if (rightp !== undefined) used_params.push(filterset_getparams('crop_right_p', rightp))
        if (modev !== undefined) used_params.push(filterset_getparams('crop_mode_v', modev))
        if (botp !== undefined) used_params.push(filterset_getparams('crop_bot_p', botp))
        if (topp !== undefined) used_params.push(filterset_getparams('crop_top_p', topp))
        return used_params
    }
    function set_graph_style_params(style, nowdot, nowtext, angle, thick, rect, grid, legendunit, drawdots)
    {
        var used_params = []
        if (style !== undefined) used_params.push(filterset_getparams('color_style', style))
        if (nowdot !== undefined) used_params.push(filterset_getparams('show_now_dot', nowdot))
        if (nowtext !== undefined) used_params.push(filterset_getparams('show_now_text', nowtext))
        if (angle !== undefined) used_params.push(filterset_getparams('angle', angle))
        if (thick !== undefined) used_params.push(filterset_getparams('thickness', thick))
        //can't figure out how to properly set rect from preset to update the VUI
        //if (rect !== undefined) used_params.push(filterset_getparams(rectProperty, rect))
        if (grid !== undefined) used_params.push(filterset_getparams('show_grid', grid))
        if (legendunit !== undefined) used_params.push(filterset_getparams('legend_unit', legendunit))
        if (drawdots !== undefined) used_params.push(filterset_getparams('draw_individual_dots', drawdots))
        return used_params
    }
    function set_graph_colors(cdot, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10)
    {
        var used_params = []
        if (cdot !== undefined) used_params.push(filterset_getparams('now_dot_color', cdot))
        if (c1 !== undefined) used_params.push(filterset_getparams('color.1', c1))
        if (c2 !== undefined) used_params.push(filterset_getparams('color.2', c2))
        if (c3 !== undefined) used_params.push(filterset_getparams('color.3', c3))
        if (c4 !== undefined) used_params.push(filterset_getparams('color.4', c4))
        if (c5 !== undefined) used_params.push(filterset_getparams('color.5', c5))
        if (c6 !== undefined) used_params.push(filterset_getparams('color.6', c6))
        if (c7 !== undefined) used_params.push(filterset_getparams('color.7', c7))
        if (c8 !== undefined) used_params.push(filterset_getparams('color.8', c8))
        if (c9 !== undefined) used_params.push(filterset_getparams('color.9', c9))
        if (c10 !== undefined) used_params.push(filterset_getparams('color.10', c10))
        return used_params
    }
    function set_graph_background_params(bgpath, scalew, opacity)
    {
        var used_params = []
        if (bgpath !== undefined) used_params.push(filterset_getparams('bg_img_path', bgpath))
        if (scalew !== undefined) used_params.push(filterset_getparams('bg_scale_w', scalew))
        return used_params
    }
}
