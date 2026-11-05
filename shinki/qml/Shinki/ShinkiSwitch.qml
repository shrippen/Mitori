import QtQuick
import QtQuick.Controls as QQC2

// Switch (bible section 5, Schalter): a square track, off `line`, on `red`; the white knob
// moves to its side, so on and off differ by more than colour. Keyboard focus: a 2 px frame in
// ink. Same interface as QQC2.Switch (text, checked, toggled()).
QQC2.Switch {
    id: control

    spacing: ShinkiStyle.gridSpace * 2
    padding: 0
    implicitWidth: text !== "" ? implicitContentWidth : implicitIndicatorWidth     // the label pads past the track
    implicitHeight: Math.max(ShinkiStyle.rowMinSpace, implicitIndicatorHeight)

    indicator: Rectangle {
        x: control.leftPadding
        y: (control.height - height) / 2
        implicitWidth: ShinkiStyle.switchWidth
        implicitHeight: ShinkiStyle.switchHeight
        color: control.checked ? ShinkiStyle.red : ShinkiStyle.line
        opacity: control.enabled ? 1 : ShinkiStyle.disabledOpacity
        border.width: control.visualFocus ? ShinkiStyle.focusWidth : 0
        border.color: ShinkiStyle.ink

        Rectangle {
            readonly property int inset: (parent.height - height) / 2

            x: control.checked ? parent.width - width - inset : inset
            y: inset
            width: ShinkiStyle.switchKnobSize
            height: width
            color: ShinkiStyle.paper

            Behavior on x {
                NumberAnimation { duration: ShinkiStyle.stateDuration }
            }
        }
    }
    contentItem: QQC2.Label {
        leftPadding: control.text !== "" ? control.indicator.width + control.spacing : 0
        visible: control.text !== ""
        text: control.text
        color: ShinkiStyle.ink
        font.family: ShinkiStyle.uiFamily
        font.pixelSize: ShinkiStyle.uiSize
        verticalAlignment: Text.AlignVCenter
    }
}
