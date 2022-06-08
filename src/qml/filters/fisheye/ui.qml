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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut
import QtQuick.Controls.Styles 1.1

Item {
    width: 300
    height: 150
    SystemPalette { id: activePalette }

    //plugin input index values
    property string focalRatioParam: '0'
    property string deFishParam: '1'
    property string lensTypeParam: '2'
    property string scaePresetParam: '3'
    property string scaleManualParam: '4'
    property string qualityParam: '5'
    property string aspectPresetParam: '6'
    property string aspectManualParam: '7'

    //default plugin values
    property double focalRatioDefault: 0.5  //default was 0.0
    property bool   deFishDefault: false
    property double lensTypeDefault: 0.6    //default #3
    property double scalePresetDefault: 0.6 //default #3
    property double scaleManualDefault: 0.5
    property double qualityDefault: 0.16
    property double aspectPresetDefault: 0.0
    property double aspectManualDefault: 0.5

    //hide user sliders
    property bool scaleShowSlider: false
    property bool aspectShowSlider: false
    property bool cameraShowNew: false    

    //combobox presets text index
    property int iDX_CAMERA: 8
    property int iDX_WIDE: 9
    property int iDX_RESULT: 10

    property var defaultParameters: [focalRatioParam,deFishParam,lensTypeParam,scaePresetParam,
                scaleManualParam,qualityParam,aspectPresetParam,aspectManualParam]


    //preset data. used for comboboxes.
    function getPresetData(index) {
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
            case  0: return [0.736, true, lensValue(4), scaleValue(1), 0.500, qualityValue(2), aspectValue(1), 0.500, 'HERO3 1080', 'Wide', 'Focus']

            //HERO4
            case  1: return [0.513, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.136, 'HERO4 1080', 'Medium', 'Action']
            case  2: return [0.643, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.487, 'HERO4 1080', 'Medium', 'Focus']
            case  3: return [0.695, true, lensValue(4), scaleValue(2), 0.500, qualityValue(2), aspectValue(1), 0.500, 'HERO4 1080', 'Medium', 'Linear']

            case  4: return [0.630, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.247, 'HERO4 1080', 'Wide', 'Action']
            case  5: return [0.714, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.468, 'HERO4 1080', 'Wide', 'Focus']
            case  6: return [0.700, true, lensValue(1), scaleValue(2), 0.500, qualityValue(2), aspectValue(1), 0.500, 'HERO4 1080', 'Wide', 'Linear']

            case  7: return [0.708, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.100, 'HERO4 1080', 'SuperView', 'Action']
            case  8: return [0.747, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.230, 'HERO4 1080', 'SuperView', 'Focus']
            case  9: return [0.750, true, lensValue(1), scaleValue(2), 0.500, qualityValue(2), aspectValue(5), 0.227, 'HERO4 1080', 'SuperView', 'Linear']

             //HERO 5
            case 10: return [0.730, true, lensValue(4), scaleValue(2), 0.500, qualityValue(2), aspectValue(1), 0.500, 'HERO5 1080', 'Wide', 'Linear']
            case 11: return [0.696, true, lensValue(4), scaleValue(2), 0.532, qualityValue(2), aspectValue(1), 0.500, 'HERO5 1080', 'SuperView', 'Linear']

            //4k CLONE
            case 12: return [0.494, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.091, '4K CLONE 1080p', 'Wide', 'Action']
            case 13: return [0.643, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.487, '4K CLONE 1080p', 'Wide', 'Focus']
            case 14: return [0.695, true, lensValue(4), scaleValue(2), 0.500, qualityValue(2), aspectValue(1), 0.500, '4K CLONE 1080p', 'Wide', 'Linear']

            case 15: return [0.442, true, lensValue(1), scaleValue(1), 0.500, qualityValue(2), aspectValue(5), 0.357, '4K CLONE 720p', 'Medium', 'Focus']
            case 16: return [0.531, true, lensValue(4), scaleValue(2), 0.500, qualityValue(2), aspectValue(5), 0.286, '4K CLONE 720p', 'Medium', 'Linear']
            //               focal, defish, lens,       scalePreset, scaleMan, interpolation,  A/R_preset,     A/R_man, camera,     qsTr(mode), qsTr(result)
        }
        //console.log('array done...')
        return '' //done
    }

    //set UI control values
    function setControlData() {
        var activeIndex        

        // #0 Focal Ratio
        focalRatioSlider.value = filter.getDouble(focalRatioParam)

        // #1 Fish or Defish
        if (filter.get(deFishParam) === '1')
             fisheyeRemoveButton.checked = true           
        else
            fisheyeAddButton.checked = true

        // #2 Mapping function (lens type)
        lensCombo.currentIndex = indexFromFloat(lensTypeParam, lensCombo.count) - 1 

        // #3 Scaling method     
        activeIndex = indexFromFloat(scaePresetParam, scaleCombo.count )
        scaleCombo.currentIndex = activeIndex - 1
        scaleShowSlider = (activeIndex == scaleCombo.count) //only show custom slider when last option used       

        // #4 Manual Scale
        scaleManualSlider.value = filter.getDouble(scaleManualParam)

       // #5 interpolation quality
        qualityCombo.currentIndex = indexFromFloat(qualityParam, qualityCombo.count) - 1

         // #6 Pixel aspect ratio presets
        activeIndex = indexFromFloat(aspectPresetParam, aspectCombo.count)
        aspectCombo.currentIndex = activeIndex - 1
        aspectShowSlider = (activeIndex == aspectCombo.count) //only show custom slider when last option used   

         // #7 Manual Pixel Aspect ratio
        aspectManualSlider.value = filter.getDouble(aspectManualParam)
    }

    //set plugin values
    function setPluginData(idx0, idx1, idx2, idx3, idx4, idx5, idx6, idx7) {
        filter.set(focalRatioParam, idx0)   // focal ratio
        filter.set(deFishParam, idx1)       // add/remove fisheye
        filter.set(lensTypeParam, idx2)     // Lens type
        filter.set(scaePresetParam, idx3)   // scale Presets
        filter.set(scaleManualParam, idx4)  // scale manual
        filter.set(qualityParam, idx5)      // Filter
        filter.set(aspectPresetParam, idx6) // A/R Presets
        filter.set(aspectManualParam, idx7) // A/R Manual
    }

    //get text string from presets
    function getPresetName(index, strIdx) {
        var v_ret = getPresetData(index)
        if (v_ret !== '') {
            if (strIdx == iDX_CAMERA)
                return v_ret[strIdx]                
            else
                return qsTr(v_ret[strIdx]) //language on 'mode' and 'results'
        }
        return v_ret
    }

    //fill combobox 'camera'
    function fillCameraCombo() {
        var a_list = []
        var v_more = true
        var v_ret = ''
        var i = 0

        do {
            v_ret = getPresetName(i, iDX_CAMERA)
            if (v_ret !== ''){
                if (a_list.indexOf(v_ret) === -1)
                    a_list.push(v_ret) //append unique camera
            } else {
                v_more = false
            }

            i += 1
        } while(v_more)

        cameraCombo.model = a_list
        fillModeCombo()

    }
    //fill combobox 'camera modes'
    function fillModeCombo() {
        var a_list = []
        var v_more = true
        var v_ret = ''
        var i = 0
        var v_cam = cameraCombo.currentText

        do {
            v_ret = getPresetName(i, iDX_CAMERA)
            if (v_ret !== '' ){
                if (v_ret === v_cam) {
                    v_ret = getPresetName(i, iDX_WIDE)
                    if (a_list.indexOf(v_ret) === -1){
                        a_list.push(v_ret) //append unique modes
                    }
                }
            } else {
                v_more = false
            }
            i += 1
        } while(v_more)

        camModeCombo.model = a_list
        fillResultsCombo()
    }

    //fill combobox 'results'
    function fillResultsCombo()  {
        var a_list = []
        var v_more = true
        var v_ret = ''
        var i = 0
        var v_cam = cameraCombo.currentText
        var v_mode = camModeCombo.currentText

        do {
            v_ret = getPresetName(i, iDX_CAMERA)
            if (v_ret !== '' ){
                if (   v_ret === v_cam //match camera
                    && v_ret === getPresetName(i, iDX_WIDE)) //match mode
                {
                    a_list.push(getPresetName(i, iDX_RESULT)) //append unique results
                }
            } else {
                v_more = false
            }
            i += 1
        } while(v_more)

        resultCombo.model = a_list 
    }

    //update plugin/ui with preset data
    function sendPresetData(index){
        var v_ret = getPresetData(index)
        setPluginData(v_ret[0], v_ret[1], v_ret[2], v_ret[3], v_ret[4], v_ret[5], v_ret[6], v_ret[7])
        setControlData()
    }
    //'Apply' button pressed
    function setCameraData(){
        var v_more = true
        var v_cam = cameraCombo.currentText
        var v_mode = camModeCombo.currentText
        var v_result = resultCombo.currentText
        var i = 0
        var v_ret = ''

        do {
            v_ret = getPresetName(i, iDX_CAMERA)
            if (v_ret !== '' ){
                if (   v_cam === v_ret //match camera
                    && v_mode === getPresetName(i, iDX_WIDE) //match mode
                    && v_result === getPresetName(i, iDX_RESULT)) //match result
                {
                    sendPresetData(i)
                    v_more = false
                }
            } else {
                v_more = false
            }
            i += 1
        } while(v_more)
    }

    //lens presets
    function lensValue(index)  { return floatFromIndex(index, lensCombo.count) }
    //scale presets
    function scaleValue(index)   { return floatFromIndex(index, scaleCombo.count) }
    //quality presets
    function qualityValue(index)  { return floatFromIndex(index, qualityCombo.count) }
    //aspect ratio presets
    function aspectValue(index)    { return floatFromIndex(index, aspectCombo.count) }

    //calculate a float for plugin. 1-based
    function floatFromIndex(idx, max) {
        if (idx < 1)
            idx = 1
        if (idx > max)
            idx = max
        return ((1 / (max-1)) * (idx-1)).toPrecision(6)
    }

    //convert float to int for UI inputs. 1-based
    function indexFromFloat(idx, max) {
        var v_rng = 1 / max
        var v_plug = filter.getDouble(idx)
        for (var i = 1; i < max; i++) {
            if (v_plug < (v_rng * i)) {
                return i
            }
        }
        return max
    }

    //index for reset button. 1-based
    function setDefaultIndex(value, max){
        var v_rng = 1 / max
        for (var i = 1; i < max; i++) {
            if (value < (v_rng * i))
                return i
        }
        return max
    }

    //set presets
    function setPresetData() {
        var v_more = true
        var v_ret = ''
        var i = 0

        do {
            v_ret = getPresetData(i)
            if (v_ret !== '' ){
                setPluginData(v_ret[0], v_ret[1], v_ret[2], v_ret[3], v_ret[4], v_ret[5], v_ret[6], v_ret[7])
                filter.savePreset(preset.parameters, qsTr(v_ret[8]+ ' ' +qsTr(v_ret[9]) + ' (' +qsTr(v_ret[10])+')'))
            } else {
                v_more = false
            }
            i += 1
        } while(v_more)    
    }

    Component.onCompleted: {
        filter.blockSignals = true
        if (filter.isNew) {
            setPresetData()
            
            // save (default) in preset dropdown
            setPluginData(focalRatioDefault, deFishDefault, lensTypeDefault, scalePresetDefault, scaleManualDefault, qualityDefault, aspectPresetDefault, aspectManualDefault)
            filter.savePreset(defaultParameters)
        }
        filter.blockSignals = false
        
        if (cameraShowNew) fillCameraCombo()

        setControlData()
    }

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
            onPresetSelected: setControlData()
        }
        Row {width: 6} //blank

        // Row 2 defish/fish
        Label {
            text: qsTr('Fisheye')
            Shotcut.HoverTip { text: qsTr('Add or remove fisheye effect') }
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            ButtonGroup { id: fisheyeGroup }
            RadioButton {
                id: fisheyeRemoveButton
                text: qsTr('Remove')
                ButtonGroup.group: fisheyeGroup
                checked: (filter.get(deFishParam) === '1')
                onCheckedChanged: { if (checked) filter.set(deFishParam, true) }
            }
            RadioButton {
                id: fisheyeAddButton
                text: qsTr('Add')
                ButtonGroup.group: fisheyeGroup
                checked: (filter.get(deFishParam) === '1') ? false:true
                onCheckedChanged: { if (checked) filter.set(deFishParam, false) }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                if(deFishDefault)
                    fisheyeRemoveButton.checked = true
                else
                    fisheyeAddButton.checked = true
            }
        }

        // Row 3: focal ratio
        Label {
            text: qsTr('Focal ratio')
            Shotcut.HoverTip { text: qsTr('The amount of lens distortion') }
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: focalRatioSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: filter.getDouble(focalRatioParam)
            onValueChanged: filter.set(focalRatioParam, value)
        }
        Shotcut.UndoButton {
            onClicked: focalRatioSlider.value = focalRatioDefault
        }

        // Row 4: Interpolation Quality
        Label {
            text: qsTr('Quality')
            Shotcut.HoverTip { text: qsTr( 'Resample quality') }
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            //Layout.columnSpan: 2
            id: qualityCombo
            implicitWidth: 120
            model: ListModel { 
                id: qualityModel 
                ListElement { text: qsTr('Nearest neighbor'); value: 0.0 }
                ListElement { text: qsTr('Bilinear'); value: 0.166 }
                ListElement { text: qsTr('Bicubic smooth'); value: 0.333 }
                ListElement { text: qsTr('Bicubic sharp'); value: 0.500 }
                ListElement { text: qsTr('Spline 4x4'); value: 0.666 }
                ListElement { text: qsTr('Spline 6x6'); value: 0.833 }                
                ListElement { text: qsTr('Lanczos 16x16'); value: 1.0 }                
            }
            textRole: 'text'
            onActivated: {
                filter.set(qualityParam, qualityModel.get(currentIndex).value)
            }
        }
         Shotcut.UndoButton {
            onClicked: {
                filter.set(qualityParam, qualityDefault)
                qualityCombo.currentIndex = setDefaultIndex(qualityDefault, qualityCombo.count) - 1
            }
        }

        // Row 5: Lens Type
        Label {
            text: qsTr('Lens type')
            Shotcut.HoverTip { text: qsTr('Select a lens distortion pattern that best matches your camera') }
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            id: lensCombo
            implicitWidth: 120
            model: ListModel { 
                id: lensModel 
                ListElement { text: qsTr('Equidistant'); value: 0.0 }
                ListElement { text: qsTr('Ortographic'); value: 0.333 }
                ListElement { text: qsTr('Equiarea'); value: 0.666 }
                ListElement { text: qsTr('Stereographic'); value: 1.0 }
            }
            textRole: 'text'
            onActivated: {
                filter.set(lensTypeParam, lensModel.get(currentIndex).value)
            }
        }
         Shotcut.UndoButton {
            onClicked: {
                filter.set(lensTypeParam, lensTypeDefault)
                lensCombo.currentIndex = setDefaultIndex(lensTypeDefault, lensCombo.count) - 1
            }
        }        
        
        //line seperator #1
        Rectangle{Layout.columnSpan: 3; Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3}

        // Row 6: Scale (Preset)
        Label {
            text: qsTr('Scale (preset)')
            Shotcut.HoverTip { text: qsTr( 'Preset scale methods\n'+
                                           'Lock pixels at specific locations') }
            Layout.alignment: Qt.AlignRight                                           
        }
        Shotcut.ComboBox {
            id: scaleCombo
            implicitWidth: 120
            model: ListModel { 
                id: scaleModel 
                ListElement { text: qsTr('Scale to Fill'); value: 0.0 }
                ListElement { text: qsTr('Keep Center Scale'); value: 0.333 }
                ListElement { text: qsTr('Scale to Fit'); value: 0.666 }
                ListElement { text: qsTr('Manual Scale'); value: 1.0 }
            }
            textRole: 'text'
            onActivated: {
                filter.set(scaePresetParam, scaleModel.get(currentIndex).value)
                scaleShowSlider = ((currentIndex+1)==scaleCombo.count) //show user input?
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.set(scaePresetParam, scalePresetDefault)
                scaleCombo.currentIndex = setDefaultIndex(scalePresetDefault, scaleCombo.count) - 1
                scaleShowSlider = false
            }
        }     

        // Row 7: Scale (Manual)
        Label {
            text: qsTr('Scale (manual)')
            Shotcut.HoverTip { text: qsTr('User set zoom/scale\nSides of image are not fixed') }
            Layout.alignment: Qt.AlignRight
            visible: scaleShowSlider
        }
        Shotcut.SliderSpinner {
            id: scaleManualSlider
            visible: scaleShowSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: filter.getDouble(scaleManualParam)
            onValueChanged: filter.set(scaleManualParam, value)
        }
        Shotcut.UndoButton {
            visible: scaleShowSlider
            onClicked: scaleManualSlider.value = scaleManualDefault
        }

        // line seperator #2
        Rectangle{Layout.columnSpan: 3; Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3}

        // Row 8: A/R (Presets)
        Label {
            text: qsTr('Aspect (preset)')
            Shotcut.HoverTip { text: qsTr( 'Preset pixel aspect ratio') }
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Shotcut.ComboBox {
                id: aspectCombo
                implicitWidth: 120
                model: ListModel { 
                    id: aspectModel 
                    ListElement { text: 'Square Pixel'; value: 0.0 }
                    ListElement { text: 'PAL DV  1.067'; value: 0.250 }
                    ListElement { text: 'NTSC DV 0.889'; value: 0.500 }
                    ListElement { text: 'HDV     1.333'; value: 0.750 }                
                    ListElement { text: qsTr('Manual Aspect'); value: 1.0 }
                }
                textRole: 'text'
                onActivated: {
                    filter.set(aspectPresetParam, aspectModel.get(currentIndex).value)
                    aspectShowSlider = ((currentIndex+1) == aspectCombo.count) //show user input?
                }
            }
        }
         Shotcut.UndoButton {
            onClicked: {
                filter.set(aspectPresetParam, aspectPresetDefault)
                aspectCombo.currentIndex = setDefaultIndex(aspectPresetDefault, aspectCombo.count) - 1
                aspectShowSlider = false
            }
        }
       
        // Row 9: A/R (Manual)
        Label {
            text: qsTr('Aspect (manual)')
            Shotcut.HoverTip { text: qsTr( 'User set pixel aspect ratios\n'+ 
                                           'Change top/side distortion bias') }
            Layout.alignment: Qt.AlignRight
            visible: aspectShowSlider
        }
        Shotcut.SliderSpinner {
            id: aspectManualSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            visible: aspectShowSlider
            value: filter.getDouble(aspectManualParam)
            onValueChanged: filter.set(aspectManualParam, value)
        }
        Shotcut.UndoButton {
            visible: aspectShowSlider
            onClicked: aspectManualSlider.value = aspectManualDefault
        }

        //line seperator #3
        Rectangle{
            visible: cameraShowNew
            Layout.columnSpan: 3; Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3}

        //row 10 combo cameras
         Label {
            visible: cameraShowNew
            text: qsTr('Camera')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            visible: cameraShowNew
            Layout.columnSpan: 2
            id: cameraCombo
            implicitWidth: 120
            model: ListModel { id: tempList1 }
            onActivated: fillModeCombo()
        }
        //row 11 combo camera mode
         Label {
            visible: cameraShowNew
            text: qsTr('Record mode')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            visible: cameraShowNew
            Layout.columnSpan: 2
            id: camModeCombo
            implicitWidth: 120
            model:  ListModel { id: tempList2 }
            onActivated: fillResultsCombo()
        }
        //row 12 combo results
        Label {
            visible: cameraShowNew
            text: qsTr('Result')
            Layout.alignment: Qt.AlignRight
        }
        Row {
            visible: cameraShowNew
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
                onClicked: setCameraData()
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
