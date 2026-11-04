import QtQuick
import QtQuick.Controls as QQC2

// A menu (the player's and the notes' entries): the style's ground with a frame of its own, the
// entries in the style's ink. The items are the controls' own, only their look is the style's.
QQC2.Menu {
    background: Rectangle {
        color: Look.backgroundColor
        border.width: Look.borderWidth
        border.color: Look.ruleColor
    }

    delegate: QQC2.MenuItem {
        id: entry

        required property var modelData

        icon.name: entry.modelData.icon && entry.modelData.icon.name ? entry.modelData.icon.name : ""
        text: entry.modelData.text
        enabled: entry.modelData.enabled
        onTriggered: entry.modelData.trigger()

        contentItem: LookLabel {
            text: entry.text
            color: entry.enabled ? Look.textColor : Look.disabledTextColor
            font: Look.defaultFont
        }
        background: Rectangle {
            color: entry.highlighted ? Look.chipColor : "transparent"
        }
    }
}
