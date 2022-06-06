/*
 * Copyright (c) 2015 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
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

/*
 * Updated UI by hypov8
 * v1 june2022 (shotcut 22.04.25)
 * -changed Interpolation quality slider to integer
 * -changed UI presets(scale + A/R) from slider to radio buttons
 * -user set sliders are hidden when scale or A/R presets are used
 * -added buttons for pre-configured cameras. eg gopro..
 * -changed UI names to be more intuitive
 * -added UI values into global varables, incase plugin changes.
 *
 * original source from forum user TwitchyMcJoe
 * https://forum.shotcut.org/t/using-defish0r-as-a-filter-code-for-a-useful-defisheye-effect/181
 *
 */


import QtQuick 2.12
import QtQuick.Controls 2.12 //1.1
import QtQuick.Layouts 1.12 //1.1
import Shotcut.Controls 1.0 as Shotcut
import QtQuick.Controls.Styles 1.1


Item {
    width: 300
    height: 150
    SystemPalette { id: activePalette }//colorGroup: SystemPalette.Active }

    //plugin input index values
    property string idx0_amount: '0'
    property string idx1_deFish: '1'
    property string idx2_lensType: '2'
    property string idx3_scaling: '3'
    property string idx4_manualScale: '4'
    property string idx5_interpolator: '5'
    property string idx6_aspectType: '6'
    property string idx7_manualAspect: '7'

    //default plugin values
    property double v0_amount: 0.5      //default was 0.0
    property bool   v1_deFish: false
    property double v2_type: 0.6        //default #3
    property double v3_scaling: 0.6     //default #3
    property double v4_manualScale: 0.5
    property double v5_interpolator: 0.16
    property double v6_aspectType: 0.0  //default #1
    property double v7_manualAspect: 0.5

    //UI option counter max  (eg.. 4 radio buttons)
    property int v2_lensMax: 4
    property int v3_scaleMax: 4
    property int v5_qualityMax: 7
    property int v6_arMax: 5
    //radioButton name arrays
    property var v2_model: ['1', '2', '3', '4']
    property var v3_model: ['1', '2', '3', 'user']
    property var v6_model: ['1', '2', '3', '4', 'user']
    //added varables above for ease of UI change, incase plugin gets updated

    //hidden user sliders
    property bool v3_scaleShow: false
    property bool v6_arShow: false

    //combobox presets text index
    property int iDX_CAMERA: 8
    property int iDX_WIDE: 9
    property int iDX_RESULT: 10

    property var defaultParameters: [idx0_amount,idx1_deFish,idx2_lensType,idx3_scaling,
                idx4_manualScale,idx5_interpolator,idx6_aspectType,idx7_manualAspect]


    //todo: add more cameras
    //preset data. used for comboboxes.
    function fn_getPreset(index) {
        switch (index) {
            //Action
            // tries to keep all 4 borders in frame. straighten left/right edge. zoomed center, makes motion look faster
            //
            //Focused
            // close match gopro studio, trimed left/right edges, centre zoom slightly, makes motion look faster
            //
            //Linear
            // minimal distortion at frame centre. edges have increased croped to suit


            //HERO3
            case  0: return [0.736, true,  f_lens[4], f_scl[1], 0.500, f_qlty[2], f_ar[1], 0.500, 'HERO-3. 1080',   'Wide',     'focused'] //todo: language?

            //HERO4
            case  1: return [0.513, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.136, 'HERO-4 1080',    'Medium',   'Action']
            case  2: return [0.643, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.487, 'HERO-4 1080',    'Medium',   'Focus']
            case  3: return [0.695, true,  f_lens(4), f_scl(2), 0.500, f_qlty(2), f_ar(1), 0.500, 'HERO-4 1080',    'Medium',   'Linear']

            case  4: return [0.630, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.247, 'HERO-4 1080',    'Wide',     'Action']
            case  5: return [0.714, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.468, 'HERO-4 1080',    'Wide',     'Focus']
            case  6: return [0.700, true,  f_lens(1), f_scl(2), 0.500, f_qlty(2), f_ar(1), 0.500, 'HERO-4 1080',    'Wide',     'Linear']

            case  7: return [0.708, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.100, 'HERO-4 1080', 'SuperView',   'Action']
            case  8: return [0.747, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.230, 'HERO-4 1080', 'SuperView',   'Focus']
            case  9: return [0.750, true,  f_lens(1), f_scl(2), 0.500, f_qlty(2), f_ar(5), 0.227, 'HERO-4 1080', 'SuperView',   'Linear']

             //HERO 5
            case  10: return [0.730, true,  f_lens(4), f_scl(2), 0.500, f_qlty(2), f_ar(1), 0.500, 'HERO-5 1080',  'Wide',     'Linear']
            case  11: return [0.696, true,  f_lens(4), f_scl(2), 0.532, f_qlty(2), f_ar(1), 0.500, 'HERO-5 1080', 'SuperView', 'Linear']

            //4k CLONE
            case  12: return [0.494, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.091, '4k-CLONE 1080p', 'None',    'Action']
            case  13: return [0.643, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.487, '4k-CLONE 1080p', 'None',    'Focus']
            case  14: return [0.695, true,  f_lens(4), f_scl(2), 0.500, f_qlty(2), f_ar(1), 0.500, '4k-CLONE 1080p', 'None',    'Linear']

            case  15: return [0.442, true,  f_lens(1), f_scl(1), 0.500, f_qlty(2), f_ar(5), 0.357, '4k-CLONE 720p', 'None',     'Focus']
            case  16: return [0.531, true,  f_lens(4), f_scl(2), 0.500, f_qlty(2), f_ar(5), 0.286, '4k-CLONE 720p', 'None',     'Linear']
            //               focal, defish, lens,      pScale,   mScale, filter,   A/R_p, A/R_m,   camera,          mode,       result
        }
        //console.log('array done...')
        return '' //done
    }

    //set UI control values
    function fn_SetControls() {

        // #0 Focal Ratio
        uID0_ratioSlider.value = filter.getDouble(idx0_amount)

        // #1 Fish or Defish
        if (filter.get(idx1_deFish) == false)
            uID1_fisheye.checked = true
        else
            uID1_defisheye.checked = true

        // #2 Mapping function (lens type)
        uID2_repeater.itemAt(fn_CalcIndex(idx2_lensType, v2_lensMax ) -1).checked = true

        // #3 Scaling method
        uID3_repeater.itemAt(fn_CalcIndex(idx3_scaling, v3_scaleMax ) -1).checked = true
        v3_scaleShow = (fn_CalcIndex(idx3_scaling, v3_scaleMax) == v3_scaleMax)? true:false

        // #4 Manual Scale
        uID4_manScaleSlider.value = filter.getDouble(idx4_manualScale)

        // #5 interpolation quality
        uID5_qualitySlider.value = fn_CalcIndex(idx5_interpolator, v5_qualityMax)

        // #6 Pixel aspect ratio presets
        uID6_repeater.itemAt(fn_CalcIndex(idx6_aspectType, v6_arMax) -1).checked = true
        v6_arShow = (fn_CalcIndex(idx6_aspectType, v6_arMax) == v6_arMax)? true:false  //show user set slider?

        // #7 Manual Pixel Aspect ratio
        uID7_manArSlider.value = filter.getDouble(idx7_manualAspect)
    }

    //set plugin values
    function fn_SetPlugin(idx0, idx1, idx2, idx3, idx4, idx5, idx6, idx7) {
        filter.set(idx0_amount, idx0)       // focal ratio
        filter.set(idx1_deFish, idx1)       // add/remove fisheye
        filter.set(idx2_lensType, idx2)     // Lens type
        filter.set(idx3_scaling, idx3)      // scale Presets
        filter.set(idx4_manualScale, idx4)  // scale manual
        filter.set(idx5_interpolator, idx5) // Filter
        filter.set(idx6_aspectType, idx6)   // A/R Presets
        filter.set(idx7_manualAspect, idx7) // A/R Manual
    }

    //get text string from presets
    function fn_getCameraListName(index, strIdx) {
       var v_ret = fn_getPreset(index)
       if (v_ret !== '')
           return v_ret[strIdx]
        return v_ret //done
    }


    //fill combobox 'camera'
    function fn_ListCamera() {
        var a_list = []
        var v_more = true
        var v_ret = ''
        var i = 0

        do {
            v_ret = fn_getCameraListName(i, iDX_CAMERA)
            if (v_ret !== ''){
                if (a_list.indexOf(v_ret) === -1)
                    a_list.push(v_ret) //append camera  if unique
            }
            else
                v_more = false

            i += 1
        } while(v_more)

        cameraCombo.model = a_list
        fn_ListFillWideModes() //fill combo 'camera modes'

    }
    //fill combobox 'camera modes'
    function fn_ListFillWideModes() {
        var a_list = []
        var v_more = true
        var v_ret = ''
        var i = 0
        var v_cam = cameraCombo.currentText

        do {
            v_ret = fn_getCameraListName(i, iDX_CAMERA)
            if (v_ret !== '' ){
                if (v_ret === v_cam) {
                    v_ret = fn_getCameraListName(i, iDX_WIDE)
                    if (a_list.indexOf(v_ret) === -1){
                        a_list.push(v_ret) //append mode if unique
                    }
                }
            }
            else {
                v_more = false
            }
            i += 1
        } while(v_more)

        camModeCombo.model = a_list
        fn_ListResultsChanged() //fill combo 'result'
    }

    //fill combobox 'results'
    function fn_ListResultsChanged()  {
        var a_list = []
        var v_more = true
        var v_ret = ''
        var i = 0
        var v_cam = cameraCombo.currentText
        var v_mode = camModeCombo.currentText

        do {
            v_ret = fn_getCameraListName(i, iDX_CAMERA)
            if (v_ret !== '' ){
                if (v_ret === v_cam){ //match camera
                    v_ret = fn_getCameraListName(i, iDX_WIDE)
                    if (v_ret === v_mode) //match mode
                        a_list.push(fn_getCameraListName(i, iDX_RESULT)) //append results if unique
                }
            }
            else {
                v_more = false
            }
            i += 1
        } while(v_more)

        resultCombo.model = a_list //["New York", "Washington", "Houston"]
    }


    //update plugin/ui with preset data
    function fn_FillInputs(index){
        var v_ret = fn_getPreset(index)
        fn_SetPlugin(v_ret[0], v_ret[1], v_ret[2], v_ret[3], v_ret[4], v_ret[5], v_ret[6], v_ret[7])
        fn_SetControls()
    }
    function fn_SetCameraData(){
        var v_more = true
        var v_cam = cameraCombo.currentText
        var v_mode = camModeCombo.currentText
        var v_result = resultCombo.currentText
        var i = 0
        var v_ret = ''

        do {
            v_ret = fn_getCameraListName(i, iDX_CAMERA)
            if (v_ret !== '' ){
                if (   v_cam === v_ret //match camera
                    && v_mode === fn_getCameraListName(i, iDX_WIDE) //match mode
                    && v_result === fn_getCameraListName(i, iDX_RESULT)) //match result
                {
                    fn_FillInputs(i)
                    v_more = false
                }
            }
            else {
                v_more = false
            }
            i += 1
        } while(v_more)
    }



    //lens presets
    function f_lens(index)  { return fn_CalcFloat(index, v2_lensMax) }
    //scale presets
    function f_scl(index)   { return fn_CalcFloat(index, v3_scaleMax) }
    //quality presets
    function f_qlty(index)  { return fn_CalcFloat(index, v5_qualityMax) }
    //aspect ratio presets
    function f_ar(index)    { return fn_CalcFloat(index, v6_arMax) }

    //calculate float for plugin
    function fn_CalcFloat(idx, max) {
        if (idx <= 0)
            idx = 1
        if (idx > max)
            idx = max
        return ((1 / (max-1)) * (idx-1)).toPrecision(6)
    }

    //convert float to int for inputs
    function fn_CalcIndex(idx, max) {
        var v_rng = 1 / max
        var v_plug = filter.getDouble(idx)
        for (var i = 1; i < max; i++) {
            if (v_plug < (v_rng * i)) {
                return i
            }
        }
        return max
    }

    //index for undo
    function fn_CalcDefaultIndex(value, max){
        var v_rng = 1 / max
        for (var i = 1; i < max; i++) {
            if (value < (v_rng * i))
                return i
        }
        return max
    }


    //loading UI done. setup values
    Component.onCompleted: {
        filter.blockSignals = true
        if (filter.isNew) {
            // save (default) in preset dropdown
            fn_SetPlugin(v0_amount, v1_deFish, v2_type, v3_scaling, v4_manualScale, v5_interpolator, v6_aspectType, v7_manualAspect)
            filter.savePreset(defaultParameters)
        }

        filter.blockSignals = false
        fn_ListCamera() //fill comboboxes

        fn_SetControls() //set UI values
    }

    ///////////////////
    //main UI gid
    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        // Row 1 Preset
        Label {
            text: qsTr('Presets')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.Preset {
            id: preset
            parameters: defaultParameters
            onPresetSelected: fn_SetControls()
        }
        Row {width: 6} //blank, keep above within grid

        //line seperator #1
        Rectangle{Layout.columnSpan: 3; Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3}

        // Row 2 defish/fish
        Label {
            text: qsTr('Fisheye')
            Shotcut.HoverTip { text: qsTr('Add or Remove fisheye effect') }
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            ButtonGroup { id: fisheyeGroup }
            RadioButton {
                id: uID1_defisheye
                text: qsTr('Remove')
                ButtonGroup.group: fisheyeGroup
                checked: (filter.get(idx1_deFish) == true) ? true:false
                onCheckedChanged: if (checked) filter.set(idx1_deFish, true)
            }
            RadioButton {
                id: uID1_fisheye
                text: qsTr('Add')
                ButtonGroup.group: fisheyeGroup
                checked: (filter.get(idx1_deFish) == false) ? true:false
                onCheckedChanged: if (checked) filter.set(idx1_deFish, false)
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                if(v1_deFish == true)
                    uID1_defisheye.checked = true
                else
                    uID1_fisheye.checked = true
            }
        }

        // Row 3: focal ratio
        Label {
            text: qsTr('Focal Ratio')
            Shotcut.HoverTip { text: qsTr('The amount of lens distortion') }
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: uID0_ratioSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: filter.getDouble(idx0_amount)
            onValueChanged: filter.set(idx0_amount, value)
        }
        Shotcut.UndoButton {
            onClicked: uID0_ratioSlider.value = v0_amount
        }

        // Row 4: Interpolation Quality
        Label {
            text: qsTr('Quality')
            Shotcut.HoverTip { text: qsTr('Resample quality\n 1=no interpolation\n 2=fast linear\n 7=slowest') }
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: uID5_qualitySlider
            minimumValue: 1
            maximumValue: v5_qualityMax //7
            decimals: 0
            suffix: ''
            value: fn_CalcIndex(idx5_interpolator,v5_qualityMax)
            onValueChanged: filter.set(idx5_interpolator, ((value - 1) / (v5_qualityMax - 1)).toPrecision(6) )
        }
        Shotcut.UndoButton {
            onClicked:  uID5_qualitySlider.value = fn_CalcDefaultIndex(v5_interpolator, v5_qualityMax)
        }

        // Row 5: Lens Type
        Label {
            text: qsTr('Lens Type')
            Shotcut.HoverTip { text: qsTr('Select a lens that best matches your camera') }
            Layout.alignment: Qt.AlignRight
        }
        Row {
            ButtonGroup { id: mapTypeGroup }
            Repeater {
                id: uID2_repeater
                model: v2_model
                RadioButton {
                    text: modelData
                    checked: (index == fn_CalcIndex(idx2_lensType, v2_lensMax) - 1)? true:false
                    ButtonGroup.group: mapTypeGroup
                    onCheckedChanged: filter.set(idx2_lensType, f_lens(index+1))
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {uID2_repeater.itemAt(fn_CalcDefaultIndex(v2_type, v2_lensMax)-1).checked = true } //2
        }

        //line seperator #2
        Rectangle{Layout.columnSpan: 3; Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3}

        // Row 6: Scale (Preset)
        Label {
            text: qsTr('Scale (Preset)')
            Layout.alignment: Qt.AlignRight
            Shotcut.HoverTip {
                text: qsTr( 'Preset Scale methods will lock pixel locations\n'+
                            'Defish\n'+
                            ' 1=fixed top-bottom\n'+
                            ' 2=fixed middle\n'+
                            ' 3=fixed corners\n'+
                            'Fisheye\n'+
                            ' 1=fixed corners\n'+
                            ' 2=fixed middle\n'+
                            ' 3=fixed top-bottom')
            }
        }
        Row {
            ButtonGroup { id: scalePresetGroup }
            Repeater {
                id: uID3_repeater
                model: v3_model
                RadioButton {
                    text: modelData
                    checked: (index == fn_CalcIndex(idx3_scaling, v3_scaleMax) - 1)? true:false
                    ButtonGroup.group: scalePresetGroup
                    onCheckedChanged: {
                        filter.set(idx3_scaling, f_scl(index+1))
                        v3_scaleShow = ((index+1)==v3_scaleMax)? true : false //show user input?
                    }
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                uID3_repeater.itemAt(fn_CalcDefaultIndex(v3_scaling, v3_scaleMax)-1).checked = true
                v3_scaleShow = false //hide user input as default
            }
        }


        // Row 7: Scale (Manual)
        Label {
            text: qsTr('Scale (Manual)')
            Shotcut.HoverTip { text: qsTr('User set zoom/scale\nNo fixed sides') }
            Layout.alignment: Qt.AlignRight
            visible: v3_scaleShow
        }
        Shotcut.SliderSpinner {
            id: uID4_manScaleSlider
            visible: v3_scaleShow
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: filter.getDouble(idx4_manualScale)
            onValueChanged: filter.set(idx4_manualScale, value)
        }
        Shotcut.UndoButton {
            visible: v3_scaleShow
            onClicked: uID4_manScaleSlider.value = v4_manualScale
        }

        // line seperator #3
        Rectangle{Layout.columnSpan: 3; Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3}

        // Row 8: A/R (Presets)
        Label {
            text: qsTr('A/R (Presets)')
            Shotcut.HoverTip { text: qsTr("Pixel Aspect Ratio presets\nUse custom when more bias is needed")}
            Layout.alignment: Qt.AlignRight
        }
        Row {
            ButtonGroup { id: arPresetGroup }
            Repeater {
                id: uID6_repeater
                model: v6_model
                RadioButton {
                    text: modelData
                    checked: (index == fn_CalcIndex(idx6_aspectType, v6_arMax) - 1)? true:false
                    ButtonGroup.group: arPresetGroup
                    onCheckedChanged: {
                        filter.set(idx6_aspectType, f_ar(index+1))
                        v6_arShow = ((index+1)==v6_arMax)? true : false
                    }
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                uID6_repeater.itemAt(fn_CalcDefaultIndex(v6_aspectType, v6_arMax)-1).checked = true
                v6_arShow = false //hide user input as default
            }
        }

        // Row 9: A/R (Manual)
        Label {
            text: qsTr('A/R (Manual)')
            Shotcut.HoverTip { text: qsTr('User set Pixel Aspect ratios\nChange top/side distortion bias')}
            Layout.alignment: Qt.AlignRight
            visible: v6_arShow
        }
        Shotcut.SliderSpinner {
            id: uID7_manArSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            visible: v6_arShow
            value: filter.getDouble(idx7_manualAspect)
            onValueChanged: filter.set(idx7_manualAspect, value)
        }
        Shotcut.UndoButton {
            visible: v6_arShow
            onClicked: uID7_manArSlider.value = v7_manualAspect
        }

        //line seperator #4
        Rectangle{Layout.columnSpan: 3; Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3}

        //row 10 combo cameras
         Label {
            text: qsTr('Camera')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            Layout.columnSpan: 2
            id: cameraCombo
            implicitWidth: 120
            model: ListModel { id: tempList1 }
            onActivated: fn_ListFillWideModes()
        }
        //row 11 combo camera mode
         Label {
            text: qsTr('Record Mode')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            Layout.columnSpan: 2
            id: camModeCombo
            implicitWidth: 120
            model:  ListModel { id: tempList2 }
            onActivated: fn_ListResultsChanged()
        }
        //row 12 combo results
        Label {
            text: qsTr('Result')
            Layout.alignment: Qt.AlignRight
        }
        Row {
            Layout.columnSpan: 2
            Shotcut.ComboBox {
                id: resultCombo
                implicitWidth: 120
                model:  ListModel { id: tempList3 }
            }
            Shotcut.Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr('Apply')
                implicitWidth: 80
                onClicked: fn_SetCameraData()
            }
        }

        //line seperator #5
        Rectangle{Layout.columnSpan: 3; Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3}


        Item {
            Layout.fillHeight: true
        }
    }
}
