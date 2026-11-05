import QtQuick
import QtQuick.Layouts

// A row of a form: the name of the setting as a small caps line above its field. The field
// itself is what the page puts in (a text field, a button, a switch).
ColumnLayout {
    id: row

    property string label: ""

    default property alias content: field.data

    Layout.fillWidth: true
    spacing: Look.unit(4)

    LookKicker {
        Layout.fillWidth: true
        color: Look.mutedTextColor      // the clickdummy's `.field label`
        text: row.label
        visible: row.label !== ""
    }
    RowLayout {
        id: field

        Layout.fillWidth: true
        spacing: Look.unit(8)
    }
}
