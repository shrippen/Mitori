import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Kante
import Mitori

// The last note on a technique and a field for a new one (player); "Notes" opens them all.
// A note begun for one technique stays with it: when the player moves on before it is added
// (training moves on after a rating), it is saved for the technique it was typed for.
ColumnLayout {
    id: noteRow

    required property var entry
    readonly property var last: Practice.notesRevision >= 0 && entry.id ? Practice.lastNote(entry.id) : ({})
    // The technique the field's text belongs to.
    property string draftFor: ""

    signal notesRequested()
    // The field got the keyboard focus (the player scrolls it into view above the keyboard).
    signal fieldFocused(Item field)

    onEntryChanged: {
        if (draftFor !== "" && draftFor !== entry.id && field.text.trim() !== "") {
            Practice.addNote(draftFor, field.text)
        }
        field.clear()
        draftFor = ""
    }

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
            onTextChanged: if (text !== "" && noteRow.draftFor === "") noteRow.draftFor = noteRow.entry.id
            onActiveFocusChanged: if (activeFocus) noteRow.fieldFocused(field)

            function add() {
                if (Practice.addNote(noteRow.draftFor || noteRow.entry.id, text)) {
                    clear()
                    noteRow.draftFor = ""
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
