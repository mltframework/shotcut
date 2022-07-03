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
    height: 228
    id: mainItemLayout
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
    property string cropParam: '8'
    property string stretchParam: '9'
    property string scaleYParam: '10'    

    //default plugin values
    property double focalRatioDefault: 0.5  //default was 0.0
    property bool   deFishDefault: true
    property double lensTypeDefault: 0.6    //default #3
    property double scalePresetDefault: 0.6 //default #3
    property double scaleManualDefault: 0.5
    property double qualityDefault: 0.16
    property double aspectPresetDefault: 0.0
    property double aspectManualDefault: 0.5
    property bool cropDefault: false
    property double stretchDefault: 0.5
    property bool superViewDefault: false
    property double scaleYDefault: 0.5    

    //hide user sliders
    property bool scaleShowSlider: false
    property bool aspectShowSlider: false
    property bool cameraShowNew: false
    property bool pluginShowNew: false    //hide newer dll features?
    property bool stretchShowSlider: false
    property bool scaleYShowSlider: false  

    //combobox presets text index
    property int iDX_CAMERA: 11
    property int iDX_WIDE: 12
    property int iDX_RESULT: 13

    property var defaultParameters: [focalRatioParam,deFishParam,lensTypeParam,scaePresetParam,
                scaleManualParam,qualityParam,aspectPresetParam,aspectManualParam, cropParam, stretchParam, scaleYParam]
    
    property bool blockUpdate: true
    
    //todo: no cropped action versions yet
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
            case  0: return [0.736, true, lensValue(4), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(1), aspectManual(0.000),  false, dynamicStretch(0.0), scaleY(0.00), 'HERO3 1080', 'Wide', 'Focus']

            //HERO4 1080 (medium)
            case  1: return [0.634, true, lensValue(1), scalePreset(4), scaleManual(0.011), qualityIndex(2), aspectPreset(5), aspectManual(-0.07), false, dynamicStretch(0.0), scaleY(0.12), 'HERO4 1080', 'Medium', 'Action']
            case  2: return [0.643, true, lensValue(1), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.01), false, dynamicStretch(0.0), scaleY(0.00), 'HERO4 1080', 'Medium', 'Focus']
            case  3: return [0.685, true, lensValue(4), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(1), aspectManual(-0.00), false, dynamicStretch(0.0), scaleY(0.00), 'HERO4 1080', 'Medium', 'Linear']
            //HERO4 1080 (wide)
            case  4: return [0.700, true, lensValue(1), scalePreset(4), scaleManual(0.019), qualityIndex(2), aspectPreset(5), aspectManual(-0.01), false, dynamicStretch(0.0), scaleY(0.23), 'HERO4 1080', 'Wide', 'Action']
            case  5: return [0.704, true, lensValue(1), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(1), aspectManual(-0.00), false, dynamicStretch(0.0), scaleY(0.00), 'HERO4 1080', 'Wide', 'Focus']
            case  6: return [0.700, true, lensValue(1), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(1), aspectManual(-0.00), false, dynamicStretch(0.0), scaleY(0.00), 'HERO4 1080', 'Wide', 'Linear']
            //HERO4 1080 (superview)
            case  7: return [0.725, true, lensValue(1), scalePreset(4), scaleManual(0.038), qualityIndex(2), aspectPreset(5), aspectManual(-0.23), false, dynamicStretch(-0.14), scaleY(0.15), 'HERO4 1080', 'SuperView', 'Action']
            case  8: return [0.737, true, lensValue(1), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.25), false, dynamicStretch(-0.14), scaleY(0.00), 'HERO4 1080', 'SuperView', 'Focus']
            case  9: return [0.725, true, lensValue(1), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.25), false, dynamicStretch(-0.14), scaleY(0.00), 'HERO4 1080', 'SuperView', 'Linear']        
            //HERO4 1440
            case 10: return [0.781, true, lensValue(4), scalePreset(4), scaleManual(0.041), qualityIndex(2), aspectPreset(5), aspectManual(-0.23), false, dynamicStretch(0.0), scaleY(0.18), 'HERO4 1440', 'Wide', 'Action']
            case 11: return [0.739, true, lensValue(1), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.21), false, dynamicStretch(0.0), scaleY(0.00), 'HERO4 1440', 'Wide', 'Focus']
            case 12: return [0.739, true, lensValue(1), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.21), false, dynamicStretch(0.0), scaleY(0.00), 'HERO4 1440', 'Wide', 'Linear']
            //HERO4 1440 4:3 (videos default saved aspect ratio)
            case 13: return [0.782, true, lensValue(4), scalePreset(4), scaleManual(0.042), qualityIndex(2), aspectPreset(5), aspectManual(-0.02), false, dynamicStretch(0.0), scaleY(0.18), 'HERO4 1440 4:3', 'Wide', 'Action']
            case 14: return [0.739, true, lensValue(1), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.03), false, dynamicStretch(0.0), scaleY(0.00), 'HERO4 1440 4:3', 'Wide', 'Focus']
            case 15: return [0.736, true, lensValue(1), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.04), false, dynamicStretch(0.0), scaleY(0.00), 'HERO4 1440 4:3', 'Wide', 'Linear']
 
            //HERO 5
            case 16: return [0.730, true, lensValue(4), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(1), aspectManual(-0.00), false, dynamicStretch(0.0), scaleY(0.00), 'HERO5 1080', 'Wide', 'Linear']
            case 17: return [0.696, true, lensValue(4), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(1), aspectManual(-0.00), false, dynamicStretch(0.0), scaleY(0.00), 'HERO5 1080', 'SuperView', 'Linear']

            //4k CLONE
            case 18: return [0.494, true, lensValue(1), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.41), false, dynamicStretch(0.0), scaleY(0.00), '4K CLONE 1080p', 'Wide', 'Action']
            case 19: return [0.643, true, lensValue(1), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.01), false, dynamicStretch(0.0), scaleY(0.00), '4K CLONE 1080p', 'Wide', 'Focus']
            case 20: return [0.695, true, lensValue(4), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(1), aspectManual(-0.00), false, dynamicStretch(0.0), scaleY(0.00), '4K CLONE 1080p', 'Wide', 'Linear']

            case 21: return [0.442, true, lensValue(1), scalePreset(1), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.14), false, dynamicStretch(0.0), scaleY(0.00), '4K CLONE 720p',  'Medium', 'Focus']
            case 22: return [0.531, true, lensValue(4), scalePreset(2), scaleManual(0.000), qualityIndex(2), aspectPreset(5), aspectManual(-0.21), false, dynamicStretch(0.0), scaleY(0.00), '4K CLONE 720p', 'Medium', 'Linear']
            //               focal, defish, lens,       scalePreset,    scaleManual,        interpolation,   A/R_preset,      A/R_manual,          crop,  fix stretch,         scaleY,        camera, qsTr(mode), qsTr(result)
        }

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
        scaleManualSlider.value = filter.getDouble(scaleManualParam) - 0.5

        // #5 interpolation quality
        qualityCombo.currentIndex = indexFromFloat(qualityParam, qualityCombo.count) - 1

        // #6 Pixel aspect ratio presets
        activeIndex = indexFromFloat(aspectPresetParam, aspectCombo.count)
        aspectCombo.currentIndex = activeIndex - 1
        aspectShowSlider = (activeIndex == aspectCombo.count) //only show custom slider when last option used

        // #7 Manual Pixel Aspect ratio
        aspectManualSlider.value = filter.getDouble(aspectManualParam) - 0.5

        // #8 crop
        cropCheckBox.checked = (filter.get(cropParam) === '1')

        // #9 stretch
        stretchSlider.value = filter.getDouble(stretchParam) - 0.5
        stretchCheckBox.checked = (stretchSlider.value !== 0.0)
        stretchShowSlider = stretchCheckBox.checked
        
        // #10 scale Y
        scaleYSlider.value = filter.getDouble(scaleYParam) - 0.5
        scaleYCheckBox.checked = (scaleYSlider.value !== 0.0)
        scaleYShowSlider = scaleYCheckBox.checked

        setScollbarHeight()
    }

    //set plugin values
    function setPluginData(idx0, idx1, idx2, idx3, idx4, idx5, idx6, idx7, idx8, idx9, idx10) {
        filter.set(focalRatioParam, idx0)   // focal ratio
        filter.set(deFishParam, idx1)       // add/remove fisheye
        filter.set(lensTypeParam, idx2)     // Lens type
        filter.set(scaePresetParam, idx3)   // scale Presets
        filter.set(scaleManualParam, idx4)  // scale manual
        filter.set(qualityParam, idx5)      // Filter
        filter.set(aspectPresetParam, idx6) // A/R Presets
        filter.set(aspectManualParam, idx7) // A/R Manual
        filter.set(cropParam, idx8)         // crop
        filter.set(stretchParam, idx9)      // stretch
        filter.set(scaleYParam, idx10)      // scaleY 
    }

    //fix scrollbar height when dynamic slider visability changes
    function setScollbarHeight(){
        mainItemLayout.height = 228
        mainItemLayout.height += stretchShowSlider? 26: 0
        mainItemLayout.height += scaleShowSlider? 26: 0
        mainItemLayout.height += aspectShowSlider? 26: 0
        mainItemLayout.height += cameraShowNew? 26*4: 0
        mainItemLayout.height += scaleYShowSlider? 26: 0
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
        var a_list = [''] //start blank
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
        
        if (v_cam === '') return

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
                if (   v_cam === v_ret //match camera
                    && v_mode === getPresetName(i, iDX_WIDE)) //match mode
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
        setPluginData(v_ret[0], v_ret[1], v_ret[2], v_ret[3], v_ret[4], v_ret[5], v_ret[6], v_ret[7], v_ret[8], v_ret[9], v_ret[10])
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
    function lensValue(index) { return floatFromIndex(index, lensCombo.count) }
    //scale presets
    function scalePreset(index) { return floatFromIndex(index, scaleCombo.count) }
    //scale manual
    function scaleManual(fValue) { return fValue + 0.5 }
    //quality presets
    function qualityIndex(index) { return floatFromIndex(index, qualityCombo.count) }
    //aspect ratio presets
    function aspectPreset(index) { return floatFromIndex(index, aspectCombo.count) }
    //aspect ratio manual
    function aspectManual(fValue) { return fValue + 0.5 }    
    //dynamic stretch
    function dynamicStretch(fValue) { return fValue + 0.5 }
    //scale Y preset
    function scaleY(fValue) { return fValue + 0.5 }    

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
                setPluginData(v_ret[0], v_ret[1], v_ret[2], v_ret[3], v_ret[4], v_ret[5], v_ret[6], v_ret[7], v_ret[8], v_ret[9], v_ret[10])
                filter.savePreset(preset.parameters, qsTr(v_ret[iDX_CAMERA]+ ' ' +qsTr(v_ret[iDX_WIDE]) + ' (' +qsTr(v_ret[iDX_RESULT])+')'))
            } else {
                v_more = false
            }
            i += 1
        } while(v_more)
    }

    Component.onCompleted: {
        if (filter.getDouble(scaleYParam) > 0.0) 
            pluginShowNew = true        
          
        filter.blockSignals = true
        if (filter.isNew) {
            setPresetData()

            // save (default) in preset dropdown
            setPluginData(focalRatioDefault, deFishDefault, lensTypeDefault, scalePresetDefault, scaleManualDefault,
                            qualityDefault, aspectPresetDefault, aspectManualDefault, cropDefault, stretchDefault, scaleYDefault)
            filter.savePreset(defaultParameters)
        }
        filter.blockSignals = false

        if (cameraShowNew) fillCameraCombo()

        setControlData()
        blockUpdate = false
    }


    GridLayout {
        columns: 3
        anchors.fill: parent
        anchors.margins: 8

        // Row 1 Preset
        Label {
            text: qsTr('Preset')
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
                checked: deFishDefault
                onCheckedChanged: {
                    if (blockUpdate) return
                    if (checked) { filter.set(deFishParam, true) }
                }
            }
            RadioButton {
                id: fisheyeAddButton
                text: qsTr('Add')
                ButtonGroup.group: fisheyeGroup
                checked: !deFishDefault
                onCheckedChanged: {
                    if (blockUpdate) return
                    if (checked) { filter.set(deFishParam, false) }
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                fisheyeRemoveButton.checked = deFishDefault
                fisheyeAddButton.checked = !deFishDefault
            }
        }

        // Row 3: focal ratio
        Label {
            text: qsTr('Focal ratio')
            leftPadding: 10
            Shotcut.HoverTip { text: qsTr('The amount of lens distortion') }
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: focalRatioSlider
            minimumValue: 0
            maximumValue: 1
            decimals: 3
            suffix: ''
            value: focalRatioDefault
            onValueChanged: { if (blockUpdate) return; filter.set(focalRatioParam, value) }
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
            id: qualityCombo
            implicitWidth: 150
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
            text: qsTr('Lens')
            Shotcut.HoverTip { text: qsTr('Select a lens distortion pattern that best matches your camera') }
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Shotcut.ComboBox {
                id: lensCombo
                implicitWidth: 150
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
            CheckBox {
                text: qsTr('Non-Linear scale') //stretch view
                leftPadding: 6
                Shotcut.HoverTip { text: qsTr(  'The image will be stretched/squished to fix camera scaling between 4:3 and 16:9\n'+
                                                'Like used in GoPro\'s superview' ) }
                id: stretchCheckBox
                padding: 0
                visible: pluginShowNew
                checked: superViewDefault
                onCheckedChanged: {
                    if (blockUpdate) return
                    stretchShowSlider = checked
                    filter.set(stretchParam, checked? stretchSlider.value+0.5: 0.5)                    
                    setScollbarHeight()
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.set(lensTypeParam, lensTypeDefault)
                lensCombo.currentIndex = setDefaultIndex(lensTypeDefault, lensCombo.count) - 1
                stretchCheckBox.checked = superViewDefault
            }
        }

        // Row 6: fix stretch
        Label {
            visible: stretchShowSlider
            text: qsTr('Scale')
            Shotcut.HoverTip { text: qsTr(  'Use negative values for up-scaled videos\n'+
                                            'Use positive values for down-scaled videos' ) }
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            visible: stretchShowSlider
            id: stretchSlider
            minimumValue: -0.5
            maximumValue: 0.5
            decimals: 2
            suffix: ''
            value: 0
            onValueChanged: { if (blockUpdate) return; filter.set(stretchParam, value+0.5) }
        }
        Shotcut.UndoButton {
            visible: stretchShowSlider
            onClicked: stretchSlider.value = stretchDefault - 0.5
        }


        //line seperator #1
         RowLayout {
            Layout.columnSpan: 3
            Label { text: qsTr('Scale')}  
            Rectangle{ Layout.fillWidth: true;height: 1; color: activePalette.text; opacity: 0.3 }
        }
        
        // Row 7: Scale (Preset)
        Label {
            text: qsTr('Preset')
            Shotcut.HoverTip { text: qsTr( 'Preset scale methods\n'+
                                           'Lock pixels at specific locations') }
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Shotcut.ComboBox {
                id: scaleCombo
                implicitWidth: 150
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
                    setScollbarHeight()
                }
            }
            CheckBox {
                text: qsTr('Y')
                leftPadding: 6
                Shotcut.HoverTip { text: qsTr('Scale Y separately\nThis changes video aspect ratio') }
                id: scaleYCheckBox
                visible: pluginShowNew                
                padding: 0
                checked: false
                onCheckedChanged: {
                    if (blockUpdate) return
                    filter.set(scaleYParam, checked? scaleYSlider.value+0.5:0.5)
                    scaleYShowSlider = checked                  
                    setScollbarHeight()
                }
            }            
            CheckBox {
                text: qsTr('Crop')
                leftPadding: 6
                Shotcut.HoverTip { text: qsTr('Remove distorted edges') }                
                id: cropCheckBox
                padding: 0
                checked: cropDefault
                visible: (pluginShowNew && fisheyeRemoveButton.checked)
                onCheckedChanged: { if (blockUpdate) return; filter.set(cropParam, checked) }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.set(scaePresetParam, scalePresetDefault)
                scaleCombo.currentIndex = setDefaultIndex(scalePresetDefault, scaleCombo.count) - 1
                scaleShowSlider = false
                cropCheckBox.checked = cropDefault
                scaleYShowSlider = false
                scaleYCheckBox.checked = false
                setScollbarHeight()
            }
        }

        // Row 8: Scale (Manual)
        Label {
            text: qsTr('Manual')
            Shotcut.HoverTip { text: qsTr('User set zoom/scale\nSides of image are not fixed') }
            Layout.alignment: Qt.AlignRight
            visible: scaleShowSlider
        }
        Shotcut.SliderSpinner {
            id: scaleManualSlider
            visible: scaleShowSlider
            minimumValue: -0.5
            maximumValue: 0.5
            decimals: 3
            suffix: ''
            value: 0
            onValueChanged: { if (blockUpdate) return; filter.set(scaleManualParam, value+0.5) }
        }
        Shotcut.UndoButton {
            visible: scaleShowSlider
            onClicked: scaleManualSlider.value = 0
        }
       
        // Row 9: Scale (Y)
        Label {
            text: qsTr('Y ratio')
            Shotcut.HoverTip { text: qsTr('Seperate Y scale') }
            Layout.alignment: Qt.AlignRight
            visible:  scaleYShowSlider
        }
        Shotcut.SliderSpinner {
            id: scaleYSlider
            visible: scaleYShowSlider
            minimumValue: -0.49
            maximumValue: 0.5
            value: 0
            decimals: 2
            suffix: ''
            onValueChanged: { if (blockUpdate) return; filter.set(scaleYParam, value+0.5) }
        }
        Shotcut.UndoButton {
            visible: scaleYShowSlider
            onClicked: scaleYSlider.value = 0
        }        

        //line seperator #2
         RowLayout {
            Layout.columnSpan: 3
            Label { text: qsTr('Aspect')}  
            Rectangle{ Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3 }
        }
        
        // Row 10: A/R (Preset)
        Label {
            text: qsTr('Preset')
            Shotcut.HoverTip { text: qsTr( 'Preset pixel aspect ratio') }
            Layout.alignment: Qt.AlignRight
        }
        RowLayout {
            Shotcut.ComboBox {
                id: aspectCombo
                implicitWidth: 150
                model: ListModel {
                    id: aspectModel
                    ListElement { text:      'Square Pixel'; value: 0.0 }
                    ListElement { text:      'PAL DV  1.067'; value: 0.250 }
                    ListElement { text:      'NTSC DV 0.889'; value: 0.500 }
                    ListElement { text:      'HDV     1.333'; value: 0.750 }
                    ListElement { text: qsTr('Manual Aspect'); value: 1.0 }
                }
                textRole: 'text'
                onActivated: {
                    filter.set(aspectPresetParam, aspectModel.get(currentIndex).value)
                    aspectShowSlider = ((currentIndex+1) == aspectCombo.count) //show user input?
                    setScollbarHeight()
                }
            }
        }
        Shotcut.UndoButton {
            onClicked: {
                filter.set(aspectPresetParam, aspectPresetDefault)
                aspectCombo.currentIndex = setDefaultIndex(aspectPresetDefault, aspectCombo.count) - 1
                aspectShowSlider = false
                setScollbarHeight()
            }
        }

        // Row 11: A/R (Manual)
        Label {
            text: qsTr('Manual')
            Shotcut.HoverTip { text: qsTr( 'User set pixel aspect ratios\n'+
                                           'Change top/side distortion bias') }
            Layout.alignment: Qt.AlignRight
            visible: aspectShowSlider
        }
        Shotcut.SliderSpinner {
            id: aspectManualSlider
            minimumValue: -0.5
            maximumValue: 0.5
            decimals: 2
            suffix: ''
            visible: aspectShowSlider
            value: 0 
            onValueChanged: { if (blockUpdate) return; filter.set(aspectManualParam, value+0.5) }
        }
        Shotcut.UndoButton {
            visible: aspectShowSlider
            onClicked: aspectManualSlider.value = 0
        }

        //line seperator #3
         RowLayout {
            visible: cameraShowNew
            Layout.columnSpan: 3
            Label { text: qsTr('Cameras')}  
            Rectangle{ Layout.fillWidth: true; height: 1; color: activePalette.text; opacity: 0.3 }
        }

        //row 12 combo cameras
         Label {
            visible: cameraShowNew
            text: qsTr('Camera')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            visible: cameraShowNew
            Layout.columnSpan: 2
            id: cameraCombo
            implicitWidth: 150
            model: ListModel { id: tempList1 }
            onActivated: fillModeCombo()
        }
        //row 13 combo camera mode
         Label {
            visible: cameraShowNew
            text: qsTr('Record mode')
            leftPadding: 10
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.ComboBox {
            visible: cameraShowNew
            Layout.columnSpan: 2
            id: camModeCombo
            implicitWidth: 150
            model:  ListModel { id: tempList2 }
            onActivated: fillResultsCombo()
        }
        //row 14 combo results
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
                implicitWidth: 150
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
