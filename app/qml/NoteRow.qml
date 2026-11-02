import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Kante
import Mitori

// The last note on a technique and a field for a new one (player); "Notes" opens them all.
ColumnLayout {
    id: noteRow

    required property var entry
    readonly property var last: Practice.notesRevision >= 0 && entry.id ? Practice.lastNote(entry.id) : ({})

    signal notesRequested()

    spacing: KanteStyle.unit(6)

    QQC2.Label {
        Layout.fillWidth: true
        visible: !!noteRow.last.text
        text: qsTr("Last note: %1").arg(noteRow.last.text || "")
        color: KanteStyle.mutedTextColor
        wrapMode: Text.Wrap
        maximumLineCount: 3
        elide: Text.ElideRight
    }
    RowLayout {
        Layout.fillWidth: true

        KanteTextField {
            id: field

            objectName: "noteField"
            Layout.fillWidth: true
            placeholderText: qsTr("Note for today …")
            Accessible.name: qsTr("New note")
            onAccepted: add()

            function add() {
                if (Practice.addNote(noteRow.entry.id, text)) {
                    clear()
                }
            }
        }
        KanteToolButton {
            enabled: field.text.trim() !== ""
            icon.name: "list-add"
            text: qsTr("Add note")
            display: QQC2.AbstractButton.IconOnly
            onClicked: field.add()
        }
        KanteToolButton {
            icon.name: "view-pim-notes"
            text: qsTr("Notes")
            display: QQC2.AbstractButton.IconOnly
            onClicked: noteRow.notesRequested()
        }
    }
}
