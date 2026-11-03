import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Button (bible section 5, `.btn`): `primary` red, white, bold; else paper with a 1 px frame and
// ink. Square. Keyboard focus: a 2 px frame in ink (on red: inset in white). `link`: the action
// of a hint, red and bold without a frame (`.novid b`).
QQC2.Button {
    id: button

    property bool primary: false
    property bool link: false

    hoverEnabled: true
    padding: ShinkiStyle.cardSpace
    implicitHeight: Math.max(ShinkiStyle.rowMinSpace, implicitContentHeight + topPadding + bottomPadding)

    background: Rectangle {
        color: button.primary ? ShinkiStyle.red : button.link ? "transparent"
             : button.hovered || button.down ? ShinkiStyle.chip : ShinkiStyle.paper
        opacity: button.enabled ? 1 : ShinkiStyle.disabledOpacity
        border.width: button.visualFocus ? ShinkiStyle.focusWidth : button.primary || button.link ? 0 : ShinkiStyle.lineWidth
        border.color: button.visualFocus && !button.primary ? ShinkiStyle.ink : button.visualFocus ? ShinkiStyle.textOnRed
                                                                                                    : ShinkiStyle.line
    }
    contentItem: RowLayout {
        spacing: ShinkiStyle.gridSpace * 2

        Kirigami.Icon {
            visible: button.icon.name !== "" && button.display !== QQC2.AbstractButton.TextOnly
            source: button.icon.name
            color: button.primary ? ShinkiStyle.textOnRed : button.link ? ShinkiStyle.red : ShinkiStyle.ink
            isMask: true
            implicitWidth: ShinkiStyle.iconSize
            implicitHeight: implicitWidth
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: button.display !== QQC2.AbstractButton.IconOnly
            text: button.text
            color: button.primary ? ShinkiStyle.textOnRed : button.link ? ShinkiStyle.red : ShinkiStyle.ink
            font.family: ShinkiStyle.uiFamily
            font.bold: button.primary || button.link
            font.pixelSize: button.primary ? ShinkiStyle.buttonSize : ShinkiStyle.uiSize
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
}
