import QtQuick
import QtQuick.Controls as QQC2

// Slider look (bible section 5, "Player"): placed inside a horizontal slider, it hides the
// style's groove and handle and draws a track of 4 px in `line`, the part up to the value in red
// and a round knob of 14 px. The slider is a row high (44 px, a finger), not the style's height.
Item {
    id: skin

    required property QQC2.Slider control
    // The way still to play (as KanteSliderSkin): on the player's dark ground the page sets it.
    property color trackColor: ShinkiStyle.line

    anchors.fill: parent
    opacity: control && control.enabled ? 1 : ShinkiStyle.disabledOpacity

    Binding {
        target: skin.control
        property: "implicitHeight"
        value: ShinkiStyle.rowMinSpace
        when: skin.control !== null
        restoreMode: Binding.RestoreBindingOrValue
    }
    Binding {
        target: skin.control ? skin.control.background : null
        property: "opacity"
        value: 0
        when: skin.control !== null && skin.control.background !== null
        restoreMode: Binding.RestoreBindingOrValue
    }
    Binding {
        target: skin.control ? skin.control.handle : null
        property: "opacity"
        value: 0
        when: skin.control !== null && skin.control.handle !== null
        restoreMode: Binding.RestoreBindingOrValue
    }

    Rectangle {
        id: track

        x: skin.control ? skin.control.leftPadding : 0
        width: skin.control ? skin.control.availableWidth : 0
        height: ShinkiStyle.sliderTrackSize
        anchors.verticalCenter: parent.verticalCenter
        color: skin.trackColor

        Rectangle {
            width: Math.round(track.width * (skin.control ? skin.control.visualPosition : 0))
            height: parent.height
            color: ShinkiStyle.red
        }
    }
    Rectangle {
        x: track.x + Math.round(track.width * (skin.control ? skin.control.visualPosition : 0)) - width / 2
        anchors.verticalCenter: parent.verticalCenter
        width: ShinkiStyle.sliderKnobSize
        height: width
        radius: width / 2
        color: ShinkiStyle.red
        border.width: skin.control && skin.control.visualFocus ? ShinkiStyle.focusWidth : 0
        border.color: ShinkiStyle.ink
    }
}
