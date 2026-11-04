import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts

// An empty place: the icon, the word and, if there is one, the explanation (the clickdummy's
// `.ph`). The style's colours, no frame of its own.
ColumnLayout {
    id: empty

    property string iconName: ""
    property string title: ""
    property string explanation: ""

    Layout.fillWidth: true
    spacing: Look.unit(8)

    IconImage {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Look.unit(32)
        Layout.preferredHeight: Look.unit(32)
        visible: empty.iconName !== ""
        name: empty.iconName
        color: Look.mutedTextColor
    }
    LookLabel {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: empty.title
        visible: empty.title !== ""
        font.bold: true
        wrapMode: Text.Wrap
    }
    LookLabel {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        text: empty.explanation
        visible: empty.explanation !== ""
        color: Look.mutedTextColor
        wrapMode: Text.Wrap
    }
}
