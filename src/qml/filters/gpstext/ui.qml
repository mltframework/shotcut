/*
 * Copyright (c) 2022 Meltytech, LLC
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
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Item {
    id: gpsTextRoot

    property url settingsOpenPath: 'file:///' + settings.openPath
    property int js_tz_offset: 0

    signal fileOpened(string path)

    function setControls() {
        textArea.text = filter.get('argument');
        textFilterUi.setControls();
        if (gpsFile.exists()) {
            fileLabel.text = gpsFile.fileName;
            fileLabelTip.text = gpsFile.filePath + "\nGPS start time: " + filter.get('gps_start_text') + "\nVideo start time: " + filter.get('video_start_text');
        } else {
            fileLabel.text = qsTr("No File Loaded.");
            fileLabel.color = 'red';
            fileLabelTip.text = qsTr('No GPS file loaded.\nClick "Open" to load a file.');
        }
        set_sec_offset_to_textfields(filter.get('time_offset'));
        combo_smoothing.currentIndex = combo_smoothing.get_smooth_index_from_val(filter.get('smoothing_value'));
        speed_multiplier.value = filter.get('speed_multiplier');
        updates_per_second.text = filter.get('updates_per_second');
        gps_processing_start.text = filter.get('gps_processing_start_time');
    }

    //this function combines the text values from sign combobox * days/hours/mins/sec TextFields into an int
    function recompute_time_offset() {
        var offset_sec = parseInt(Number(offset_days.text), 10) * 24 * 60 * 60 + parseInt(Number(offset_hours.text), 10) * 60 * 60 + parseInt(Number(offset_mins.text), 10) * 60 + parseInt(Number(offset_secs.text), 10);
        offset_sec *= parseInt(filter.get('majoroffset_sign'), 10);
        filter.set('time_offset', Number(offset_sec).toFixed(0));
    }

    //transforms a wheel-up/down event into the correct offset direction
    function wheel_offset(val) {
        var offset = Number(filter.get("time_offset"));
        if (offset < 0)
            val *= -1;
        if (offset == 0 && val < 0)
            return;

        //fix unnatural behaviour when subtracting at offset 0
        set_sec_offset_to_textfields(offset + val);
    }

    //splits (and fills) seconds into days/hours/mins/secs textfields
    function set_sec_offset_to_textfields(secs) {
        if (secs === '')
            return;
        if (secs < 0) {
            combo_majoroffset_sign.currentIndex = 1;
            filter.set('majoroffset_sign', -1);
        } else {
            combo_majoroffset_sign.currentIndex = 0;
            filter.set('majoroffset_sign', 1);
        }
        offset_days.text = parseInt(Math.abs(secs) / (24 * 60 * 60), 10);
        offset_hours.text = parseInt(Math.abs(secs) / (60 * 60) % 24, 10);
        offset_mins.text = parseInt(Math.abs(secs) / 60 % 60, 10);
        offset_secs.text = parseInt(Math.abs(secs) % 60, 10);
        //toFixed(0) avoids scientific notation!
        filter.set('time_offset', Number(secs).toFixed(0));
    }

    width: 300
    height: 800
    onFileOpened: path => {
        settings.openPath = path;
    }
    Component.onCompleted: {
        var resource = filter.get('resource');
        if (!resource)
            resource = filter.get('gps.file');
        gpsFile.url = resource;
        filter.blockSignals = true;
        filter.set(textFilterUi.middleValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.startValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.endValue, Qt.rect(0, 0, profile.width, profile.height));
        if (filter.isNew) {
            var presetParams = preset.parameters.slice();
            var index = presetParams.indexOf('argument');
            if (index > -1)
                presetParams.splice(index, 1);
            if (application.OS === 'Windows')
                filter.set('family', 'Verdana');
            filter.set('fgcolour', '#ffffffff');
            filter.set('bgcolour', '#00000000');
            filter.set('olcolour', '#aa000000');
            filter.set('outline', 3);
            filter.set('weight', 10 * Font.Normal);
            filter.set('style', 'normal');
            filter.set(textFilterUi.useFontSizeProperty, false);
            filter.set('size', profile.height);
            // Add default preset.
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'left');
            filter.set(textFilterUi.rectProperty, '10%/10%:80%x80%');
            filter.savePreset(presetParams);
            filter.set(textFilterUi.rectProperty, filter.getRect(textFilterUi.rectProperty));
        } else {
            filter.set(textFilterUi.middleValue, filter.getRect(textFilterUi.rectProperty, filter.animateIn + 1));
            if (filter.animateIn > 0)
                filter.set(textFilterUi.startValue, filter.getRect(textFilterUi.rectProperty, 0));
            if (filter.animateOut > 0)
                filter.set(textFilterUi.endValue, filter.getRect(textFilterUi.rectProperty, filter.duration - 1));
        }
        //gps properties
        if (filter.isNew) {
            filter.set('time_offset', 0);
            filter.set('majoroffset_sign', 1);
            filter.set('smoothing_value', 5);
            filter.set('videofile_timezone_seconds', 0);
            filter.set('speed_multiplier', 1);
            filter.set('updates_per_second', 1);
            filter.set('gps_processing_start_time', 'yyyy-MM-dd hh:mm:ss');
        } else {
            if (filter.get('gps_processing_start_time') == 'yyyy-MM-dd hh:mm:ss' && filter.get('gps_start_text') != '')
                filter.set('gps_processing_start_time', filter.get('gps_start_text'));
        }
        filter.blockSignals = false;
        //get current timezone
        var date = new Date();
        js_tz_offset = date.getTimezoneOffset() * 60;
        setControls();
    }

    Shotcut.File {
        id: gpsFile
    }

    FileDialog {
        id: fileDialog

        modality: application.dialogModality
        currentFolder: settingsOpenPath
        nameFilters: ['Supported files (*.gpx *.tcx)', 'GPS Exchange Format (*.gpx)', 'Training Center XML (*.tcx)']
        onAccepted: {
            gpsFile.url = fileDialog.selectedFile;
            gpsTextRoot.fileOpened(gpsFile.path);
            fileLabel.text = gpsFile.fileName;
            fileLabel.color = activePalette.text;
            fileLabelTip.text = gpsFile.filePath;
            filter.set('resource', gpsFile.url);
            filter.set('gps_start_text', '');
            filter.set('gps_processing_start_time', 'yyyy-MM-dd hh:mm:ss');
            gpsFinishParseTimer.restart();
        }
    }

    //timer to update UI after gps file is processed; max: 10x250ms
    Timer {
        id: gpsFinishParseTimer

        property int calls: 0

        interval: 250
        repeat: true
        triggeredOnStart: false
        onTriggered: {
            if (filter.get('gps_start_text') == '') {
                calls += 1;
                if (calls > 10) {
                    gpsFinishParseTimer.stop();
                    calls = 0;
                    filter.set('gps_processing_start_time', 'yyyy-MM-dd hh:mm:ss');
                    setControls();
                }
            } else {
                gpsFinishParseTimer.stop();
                calls = 0;
                filter.set('gps_processing_start_time', filter.get('gps_start_text'));
                setControls();
            }
        }
    }

    GridLayout {
        id: mainGrid

        columns: 2
        anchors.fill: parent
        anchors.margins: 8
        width: 300

        Shotcut.Button {
            id: openButton

            text: qsTr('Open file')
            Layout.alignment: Qt.AlignRight
            onClicked: {
                fileDialog.title = qsTr("Open GPS File");
                fileDialog.open();
            }
        }

        Label {
            id: fileLabel

            Layout.fillWidth: true

            Shotcut.HoverTip {
                id: fileLabelTip
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

            Shotcut.HoverTip {
                text: qsTr('This is added to video time to sync with gps time.')
            }
        }

        RowLayout {
            Shotcut.ComboBox {
                id: combo_majoroffset_sign

                implicitWidth: 39
                currentIndex: 0
                textRole: 'text'
                onActivated: {
                    filter.set('majoroffset_sign', sign_val.get(currentIndex).value);
                    recompute_time_offset();
                }

                Shotcut.HoverTip {
                    text: qsTr('+ : Adds time to video (use if GPS is ahead).\n - : Subtracts time from video (use if video is ahead).')
                }

                model: ListModel {
                    id: sign_val

                    ListElement {
                        text: '+'
                        value: 1
                    }

                    ListElement {
                        text: '-'
                        value: -1
                    }
                }
            }

            TextField {
                id: offset_days

                text: '0'
                horizontalAlignment: TextInput.AlignRight
                implicitWidth: 60
                onFocusChanged: {
                    if (focus)
                        selectAll();
                }
                onTextChanged: {
                    if (!acceptableInput)
                        offset_days.undo();
                }
                onEditingFinished: recompute_time_offset()

                MouseArea {
                    anchors.fill: parent
                    onWheel: wheel_offset(wheel.angleDelta.y > 0 ? 86400 : -86400)
                    onClicked: offset_days.forceActiveFocus()
                }

                Shotcut.HoverTip {
                    text: qsTr('Number of days to add/subtract to video time to sync them.\nTip: you can use mousewheel to change values.')
                }

                validator: IntValidator {
                    bottom: 0
                    top: 36600
                }
            }

            Label {
                text: ':'
                Layout.alignment: Qt.AlignRight
            }

            TextField {
                id: offset_hours

                text: '0'
                horizontalAlignment: TextInput.AlignRight
                implicitWidth: 30
                onFocusChanged: {
                    if (focus)
                        selectAll();
                }
                onTextChanged: {
                    if (!acceptableInput)
                        offset_hours.undo();
                }
                onEditingFinished: recompute_time_offset()

                MouseArea {
                    anchors.fill: parent
                    onWheel: wheel_offset(wheel.angleDelta.y > 0 ? 3600 : -3600)
                    onClicked: {
                        offset_hours.forceActiveFocus();
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Number of hours to add/subtract to video time to sync them.\nTip: you can use mousewheel to change values.')
                }

                validator: IntValidator {
                    bottom: 0
                    top: 23
                }
            }

            Label {
                text: ':'
                Layout.alignment: Qt.AlignRight
            }

            TextField {
                id: offset_mins

                text: '0'
                horizontalAlignment: TextInput.AlignRight
                implicitWidth: 30
                onFocusChanged: {
                    if (focus)
                        selectAll();
                }
                onTextChanged: {
                    if (!acceptableInput)
                        offset_mins.undo();
                }
                onEditingFinished: recompute_time_offset()

                MouseArea {
                    anchors.fill: parent
                    onWheel: wheel_offset(wheel.angleDelta.y > 0 ? 60 : -60)
                    onClicked: {
                        offset_mins.forceActiveFocus();
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Number of minutes to add/subtract to video time to sync them.\nTip: you can use mousewheel to change values.')
                }

                validator: IntValidator {
                    bottom: 0
                    top: 59
                }
            }

            Label {
                text: ':'
                Layout.alignment: Qt.AlignRight
            }

            TextField {
                id: offset_secs

                text: '0'
                horizontalAlignment: TextInput.AlignRight
                implicitWidth: 30
                onFocusChanged: {
                    if (focus)
                        selectAll();
                }
                onTextChanged: {
                    if (!acceptableInput)
                        offset_secs.undo();
                }
                onEditingFinished: recompute_time_offset()

                MouseArea {
                    anchors.fill: parent
                    onWheel: wheel_offset(wheel.angleDelta.y > 0 ? 1 : -1)
                    onClicked: {
                        offset_secs.forceActiveFocus();
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Number of seconds to add/subtract to video time to sync them.\nTip: you can use mousewheel to change values.')
                }

                validator: IntValidator {
                    bottom: 0
                    top: 59
                }
            }

            //buttons:
            Shotcut.Button {
                icon.name: 'media-skip-backward'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    set_sec_offset_to_textfields(filter.get('auto_gps_offset_start'));
                }

                Shotcut.HoverTip {
                    text: qsTr('Sync start of GPS to start of video file.\nTip: use this if you started GPS and video recording at the same time.')
                }
            }

            Shotcut.Button {
                icon.name: 'document-open-recent'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    set_sec_offset_to_textfields(parseInt(Number(filter.get('time_offset'))) + parseInt(Number(js_tz_offset)));
                }

                Shotcut.HoverTip {
                    text: qsTr('Remove timezone (%1 seconds) time from video file (convert to UTC).\nTip: use this if your video camera doesn\'t have timezone settings as it will set local time as UTC.'.arg(js_tz_offset))
                }
            }

            Shotcut.Button {
                icon.name: 'format-indent-less'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    set_sec_offset_to_textfields(parseInt(Number(filter.get('time_offset')) - producer.length / profile.fps));
                }

                Shotcut.HoverTip {
                    text: qsTr('Fix video start time: if file time is actually end time, press this button to subtract file length (%1 seconds) from GPS offset.'.arg(parseInt(producer.length / profile.fps)))
                }
            }

            Shotcut.Button {
                icon.name: 'media-playback-pause'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    set_sec_offset_to_textfields(filter.get('auto_gps_offset_now'));
                }

                Shotcut.HoverTip {
                    text: qsTr('Sync start of GPS to current video time.\nTip: use this if you recorded the moment of the first GPS fix.')
                }
            }

            Shotcut.UndoButton {
                onClicked: set_sec_offset_to_textfields(0)
            }
        }

        Label {
            text: qsTr('GPS smoothing')
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Average nearby GPS points to smooth out errors.')
            }
        }

        RowLayout {
            Shotcut.ComboBox {
                id: combo_smoothing

                function get_smooth_index_from_val(val) {
                    for (var i = 0; i < smooth_val_list.count; i++) {
                        if (smooth_val_list.get(i).value == val)
                            return i;
                    }
                    console.log("get_smooth_index_from_val: no match for smooth val= " + val);
                    return 3; //default
                }

                implicitWidth: 300
                textRole: 'text'
                currentIndex: 3
                onActivated: {
                    filter.set('smoothing_value', smooth_val_list.get(currentIndex).value);
                }

                model: ListModel {
                    id: smooth_val_list

                    ListElement {
                        text: '0 (raw data)'
                        value: 0
                    }

                    ListElement {
                        text: '1 (interpolate and process data)'
                        value: 1
                    }

                    ListElement {
                        text: '3 points'
                        value: 3
                    }

                    ListElement {
                        text: '5 points'
                        value: 5
                    }

                    ListElement {
                        text: '7 points'
                        value: 7
                    }

                    ListElement {
                        text: '11 points'
                        value: 11
                    }

                    ListElement {
                        text: '15 points'
                        value: 15
                    }

                    ListElement {
                        text: '31 points'
                        value: 31
                    }

                    ListElement {
                        text: '63 points'
                        value: 63
                    }

                    ListElement {
                        text: '127 points'
                        value: 127
                    }
                }
            }

            Shotcut.UndoButton {
                onClicked: {
                    combo_smoothing.currentIndex = 3;
                    filter.set('smoothing_value', 5);
                }
            }
        }

        Label {
            text: qsTr('Processing start')
            leftPadding: 10
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Distances are calculated since the start of the gps file, use this field to reset them (GPS time).')
            }
        }

        RowLayout {
            TextField {
                id: gps_processing_start

                text: 'yyyy-MM-dd hh:mm:ss'
                horizontalAlignment: TextInput.AlignRight
                //TODO: regex to validate date yyyy-MM-dd hh:mm:ss
                implicitWidth: 128
                selectByMouse: true
                persistentSelection: true
                onEditingFinished: filter.set('gps_processing_start_time', gps_processing_start.text)

                Shotcut.HoverTip {
                    text: qsTr('Insert date and time formatted exactly as: YYYY-MM-DD HH:MM:SS (GPS time).')
                }
            }

            Shotcut.Button {
                icon.source: 'qrc:///icons/dark/32x32/media-playback-pause'
                implicitWidth: 20
                implicitHeight: 20
                onClicked: {
                    var gps_time_now = filter.get('auto_gps_processing_start_now');
                    gps_processing_start.text = gps_time_now;
                    filter.set('gps_processing_start_time', gps_time_now);
                }

                Shotcut.HoverTip {
                    text: qsTr('Set start of GPS processing to current video time.')
                }
            }

            Shotcut.UndoButton {
                onClicked: {
                    gps_processing_start.text = filter.get('gps_start_text');
                    filter.set('gps_processing_start_time', filter.get('gps_start_text'));
                }
            }
        }

        Label {
            text: qsTr('Video speed')
            leftPadding: 10
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('If the current video is sped up (timelapse) or slowed down use this field to set the speed.')
            }
        }

        RowLayout {
            Shotcut.DoubleSpinBox {
                id: speed_multiplier

                value: 1
                horizontalAlignment: Qt.AlignRight
                Layout.minimumWidth: 80
                from: 0
                to: 1000
                decimals: 2
                stepSize: 1
                suffix: 'x'
                onValueChanged: {
                    filter.set('speed_multiplier', value);
                }

                Shotcut.HoverTip {
                    text: qsTr('Fractional times are also allowed (0.25 = 4x slow motion, 5 = 5x timelapse).')
                }
            }

            Shotcut.UndoButton {
                onClicked: {
                    filter.set('speed_multiplier', 1);
                    speed_multiplier.value = 1;
                }
            }
        }

        Label {
            topPadding: 10
            text: qsTr('<b>Text options</b>')
            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: textFilterUi.parameterList.concat(['argument'])
            onBeforePresetLoaded: {
                filter.resetProperty(textFilterUi.rectProperty);
            }
            onPresetSelected: {
                setControls();
                filter.blockSignals = true;
                filter.set(textFilterUi.middleValue, filter.getRect(textFilterUi.rectProperty, filter.animateIn + 1));
                if (filter.animateIn > 0)
                    filter.set(textFilterUi.startValue, filter.getRect(textFilterUi.rectProperty, 0));
                if (filter.animateOut > 0)
                    filter.set(textFilterUi.endValue, filter.getRect(textFilterUi.rectProperty, filter.duration - 1));
                filter.blockSignals = false;
            }
        }

        Label {
            text: qsTr('Text')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
        }

        Item {
            Layout.minimumHeight: fontMetrics.height * 6
            Layout.maximumHeight: Layout.minimumHeight
            Layout.minimumWidth: preset.width
            Layout.maximumWidth: preset.width

            FontMetrics {
                id: fontMetrics

                font: textArea.font
            }

            ScrollView {
                id: scrollview

                width: preset.width - (ScrollBar.vertical.visible ? 16 : 0)
                height: parent.height - (ScrollBar.horizontal.visible ? 16 : 0)
                clip: true

                TextArea {
                    id: textArea

                    property int maxLength: 1024

                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.NoWrap
                    selectByMouse: true
                    persistentSelection: true
                    padding: 0
                    text: '__empty__'
                    onTextChanged: {
                        if (text === '__empty__')
                            return;
                        if (length > maxLength) {
                            text = text.substring(0, maxLength);
                            cursorPosition = maxLength;
                        }
                        if (!parseInt(filter.get(textFilterUi.useFontSizeProperty), 10))
                            filter.set('size', profile.height / text.split('\n').length);
                        filter.set('argument', text);
                    }

                    MouseArea {
                        acceptedButtons: Qt.RightButton
                        anchors.fill: parent
                        onClicked: contextMenu.popup()
                    }

                    Shotcut.EditMenu {
                        id: contextMenu
                    }

                    background: Rectangle {
                        anchors.fill: parent
                        color: textArea.palette.base
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    height: 16
                    policy: ScrollBar.AlwaysOn
                    visible: scrollview.contentWidth > scrollview.width
                    parent: scrollview.parent
                    anchors.top: scrollview.bottom
                    anchors.left: scrollview.left
                    anchors.right: scrollview.right
                }

                ScrollBar.vertical: ScrollBar {
                    width: 16
                    policy: ScrollBar.AlwaysOn
                    visible: scrollview.contentHeight > scrollview.height
                    parent: scrollview.parent
                    anchors.top: scrollview.top
                    anchors.left: scrollview.right
                    anchors.bottom: scrollview.bottom
                }
            }
        }

        Label {
            text: qsTr('Insert GPS field')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Layout.bottomMargin: 5

            Shotcut.ComboBox {

                //0
                //1
                //2
                //3
                //4
                //5
                //6
                //7
                //8
                //9
                //10
                //11
                //12
                //13
                //14
                implicitWidth: 300
                model: [qsTr('GPS latitude'), qsTr('GPS longitude'), qsTr('Elevation (m)'), qsTr('Speed (km/h)'), qsTr('Distance (m)'), qsTr('GPS date-time'), qsTr('Video file date-time'), qsTr('Heart-rate (bpm)'), qsTr('Bearing (degrees)'), qsTr('Bearing (compass)'), qsTr('Elevation gain (m)'), qsTr('Elevation loss (m)'), qsTr('Distance uphill (m)'), qsTr('Distance downhill (m)'), qsTr('Distance flat (m)')]
                onActivated: {
                    switch (currentIndex) {
                    case 0:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_lat#');
                        break;
                    case 1:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_lon#');
                        break;
                    case 2:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_elev m#');
                        break;
                    case 3:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_speed kmh#');
                        break;
                    case 4:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_dist m#');
                        break;
                    case 5:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_datetime_now#');
                        break;
                    case 6:
                        onClicked: textArea.insert(textArea.cursorPosition, '#file_datetime_now#');
                        break;
                    case 7:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_hr#');
                        break;
                    case 8:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_bearing#');
                        break;
                    case 9:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_compass#');
                        break;
                    case 10:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_vdist_up#');
                        break;
                    case 11:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_vdist_down#');
                        break;
                    case 12:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_dist_uphill#');
                        break;
                    case 13:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_dist_downhill#');
                        break;
                    case 14:
                        onClicked: textArea.insert(textArea.cursorPosition, '#gps_dist_flat#');
                        break;
                    default:
                        console.log('gps_combobox: current index not supported: ' + currentIndex);
                    }
                }

                Shotcut.HoverTip {
                    text: qsTr('Extra arguments can be added inside keywords:\nDistance units: m [km|ft|mi].\nSpeed units: km/h [mi/h|m/s|ft/s].\nTime default: %Y-%m-%d %H:%M:%S, extra offset can be added as +/-seconds (+3600).\nExtra keyword: RAW (prints only values from file).')
                }
            }
        }

        Shotcut.TextFilterUi {
            id: textFilterUi

            Layout.leftMargin: 10
            Layout.columnSpan: 2
        }

        Label {
            topPadding: 10
            text: qsTr('<b>Advanced options</b>')
            Layout.columnSpan: 2
        }

        Label {
            text: qsTr('Update speed')
            leftPadding: 10
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Set how many text updates to show per second.\nSet to 0 to only print real points (no interpolation).')
            }
        }

        RowLayout {
            TextField {
                id: updates_per_second

                text: '1'
                horizontalAlignment: TextInput.AlignRight
                implicitWidth: 25
                onFocusChanged: {
                    if (focus)
                        selectAll();
                }
                onEditingFinished: filter.set('updates_per_second', updates_per_second.text)

                Shotcut.HoverTip {
                    text: qsTr('Fractional times are also allowed (0.25 = update every 4 seconds, 5 = 5 updates per second).')
                }

                validator: DoubleValidator {
                    bottom: 0
                    top: 1000
                }
            }

            Label {
                text: qsTr(' per second')
            }

            Shotcut.UndoButton {
                onClicked: {
                    filter.set('updates_per_second', 1);
                    updates_per_second.text = '1';
                }
            }
        }

        Rectangle {
            Layout.columnSpan: parent.columns
            Layout.fillWidth: true
            Layout.minimumHeight: 12
            color: 'transparent'

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 2
                radius: 2
                color: activePalette.text
            }
        }

        Label {
            text: qsTr('Video start time:')
            leftPadding: 10
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Detected date-time for the video file.')
            }
        }

        Label {
            id: video_start

            text: filter.get('video_start_text')
            Layout.alignment: Qt.AlignLeft

            Shotcut.HoverTip {
                text: "This time will be used for synchronization."
            }
        }

        Label {
            id: start_location_datetime

            text: qsTr('GPS start time:')
            leftPadding: 10
            Layout.alignment: Qt.AlignRight

            Shotcut.HoverTip {
                text: qsTr('Detected date-time for the GPS file.')
            }
        }

        Label {
            id: gps_start

            text: filter.get('gps_start_text')
            Layout.alignment: Qt.AlignLeft

            Shotcut.HoverTip {
                text: qsTr('This time will be used for synchronization.')
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
