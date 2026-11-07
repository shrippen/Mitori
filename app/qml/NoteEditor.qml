import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import Mitori

// Change or delete one note (from the notes page, press and hold). A technique's note by
// default; a course notebook gives its own `save` and `remove` (Courses).
Kirigami.Dialog {
    id: editor

    property var note: ({})
    property var save: (uid, text) => Practice.changeNote(uid, text)
    property var remove: uid => Practice.deleteNote(uid)

    function edit(n) {
        note = n
        area.text = n.text
        open()
        area.forceActiveFocus()
    }

    objectName: "noteEditor"
    title: qsTr("Note")
    preferredWidth: Kirigami.Units.gridUnit * 24
    padding: Kirigami.Units.largeSpacing
    standardButtons: Kirigami.Dialog.NoButton
    customFooterActions: [
        Kirigami.Action {
            icon.name: "edit-delete"
            text: qsTr("Delete")
            onTriggered: {
                if (editor.remove(editor.note.uid)) {
                    editor.close()
                }
            }
        },
        Kirigami.Action {
            icon.name: "document-save"
            text: qsTr("Save")
            enabled: area.text.trim() !== "" || !!editor.note.media      // a photo needs no text
            onTriggered: {
                if (editor.save(editor.note.uid, area.text)) {
                    editor.close()
                }
            }
        }
    ]

    // Not a content child: the dialog's scroll view sizes itself by its only child.
    property Item lookScope: LookScope { target: editor.contentItem }

    // Closed with a changed text and not saved (Escape, Back, ×, a tap beside it): asks; Cancel
    // opens the editor again with the change. A text emptied keeps the note as it was (Delete
    // removes it), unless the note has a photo or voice note to stand on.
    readonly property bool changed: area.text.trim() !== (note.text || "").trim()
                                    && (area.text.trim() !== "" || !!note.media)
    onRejected: {
        if (changed) {
            leaveAsk.open()
        }
    }
    // The question's text (a notebook says "entry").
    property string changedText: qsTr("The note has been changed but not saved.")
    // Not a content child either; on the page, beside the editor. Main asks it too when the window
    // closes with the editor open (`asking`); Save and Discard close the editor then.
    readonly property LeaveDialog leaveAsk: LeaveDialog {
        objectName: "leaveChange"
        parent: editor.parent
        asking: editor.opened && editor.changed
        title: qsTr("Save the change?")
        text: editor.changedText
        save: () => {
            if (!editor.save(editor.note.uid, area.text)) {
                return false
            }
            editor.close()
            return true
        }
        discard: () => editor.close()
        onStayed: editor.open()
    }

    QQC2.TextArea {
        id: area

        objectName: "noteArea"
        wrapMode: TextEdit.Wrap
        Accessible.name: qsTr("Note")
    }
}
