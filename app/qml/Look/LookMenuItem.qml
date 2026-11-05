import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Controls.impl
import QtQuick.Layouts

// An entry of a menu: its symbol and word in the style's ink, the style's ground under the pointer.
QQC2.MenuItem {
    id: entry

    contentItem: RowLayout {
        spacing: Look.unit(8)

        IconImage {
            Layout.preferredWidth: Look.unit(16)
            Layout.preferredHeight: Look.unit(16)
            visible: entry.icon.name !== ""
            name: entry.icon.name
            color: entry.enabled ? Look.textColor : Look.disabledTextColor
        }
        LookLabel {
            Layout.fillWidth: true
            text: entry.text
            color: entry.enabled ? Look.textColor : Look.disabledTextColor
        }
    }
    background: Rectangle {
        color: entry.highlighted ? Look.chipColor : "transparent"
    }
}
