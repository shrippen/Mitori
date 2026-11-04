import QtQuick
import QtQuick.Controls as QQC2

// An entry of a menu (the notes' and the player's): the style's ink, the style's ground while
// the pointer is on it.
QQC2.MenuItem {
    id: entry

    contentItem: LookLabel {
        text: entry.text
        color: entry.enabled ? Look.textColor : Look.disabledTextColor
        font: Look.defaultFont
    }
    background: Rectangle {
        color: entry.highlighted ? Look.chipColor : "transparent"
    }
}
