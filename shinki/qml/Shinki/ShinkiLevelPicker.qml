import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Rating (bible section 5, `.rate`): `count` equal fields, the number bold over its word; the
// chosen one filled red with white text. Left and Right move, Enter or Space picks.
//   value      the level now (0: not rated)
//   names      the words, index = level
//   picked(l)  a level was chosen
FocusScope {
    id: picker

    property int count: 4
    property int value: 0
    property var names: []
    property int focusLevel: Math.max(1, value)
    signal picked(int level)

    implicitHeight: row.implicitHeight
    implicitWidth: row.implicitWidth
    activeFocusOnTab: true

    Accessible.role: Accessible.RadioButton

    Keys.onLeftPressed: picker.focusLevel = Math.max(1, picker.focusLevel - 1)
    Keys.onRightPressed: picker.focusLevel = Math.min(picker.count, picker.focusLevel + 1)
    Keys.onReturnPressed: picker.picked(picker.focusLevel)
    Keys.onEnterPressed: picker.picked(picker.focusLevel)
    Keys.onSpacePressed: picker.picked(picker.focusLevel)

    RowLayout {
        id: row

        anchors.fill: parent
        spacing: ShinkiStyle.gridSpace * 2

        Repeater {
            model: picker.count

            QQC2.AbstractButton {
                id: field

                required property int index
                readonly property int level: index + 1
                readonly property bool chosen: picker.value === level
                readonly property bool focused: picker.activeFocus && picker.focusLevel === level

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                focusPolicy: Qt.NoFocus
                hoverEnabled: true
                padding: ShinkiStyle.gridSpace * 2
                Accessible.name: level + " " + (picker.names[level] || "")
                Accessible.checked: chosen
                onClicked: picker.picked(level)

                background: Rectangle {
                    color: field.chosen ? ShinkiStyle.red : field.hovered ? ShinkiStyle.chip : ShinkiStyle.paper
                    border.width: field.focused ? ShinkiStyle.focusWidth : ShinkiStyle.lineWidth
                    border.color: field.focused ? ShinkiStyle.ink : field.chosen ? ShinkiStyle.red : ShinkiStyle.line
                }
                contentItem: ColumnLayout {
                    spacing: 0

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: field.level
                        color: field.chosen ? ShinkiStyle.textOnRed : ShinkiStyle.ink
                        font.family: ShinkiStyle.uiFamily
                        font.bold: true
                        font.pixelSize: ShinkiStyle.rateNumberSize
                        horizontalAlignment: Text.AlignHCenter
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: picker.names[field.level] || ""
                        color: field.chosen ? ShinkiStyle.textOnRed : ShinkiStyle.inkMuted
                        font.family: ShinkiStyle.uiFamily
                        font.pixelSize: ShinkiStyle.rateWordSize
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
