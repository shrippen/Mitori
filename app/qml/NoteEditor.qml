import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Change or delete one note (from the notes page, press and hold).
Kirigami.Dialog {
    id: editor

    property var note: ({})

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
                if (Practice.deleteNote(editor.note.uid)) {
                    editor.close()
                }
            }
        },
        Kirigami.Action {
            icon.name: "document-save"
            text: qsTr("Save")
            enabled: area.text.trim() !== ""
            onTriggered: {
                if (Practice.changeNote(editor.note.uid, area.text)) {
                    editor.close()
                }
            }
        }
    ]

    // Not a content child: the dialog's scroll view sizes itself by its only child.
    property Item kanteScope: KanteScope { target: editor.contentItem }

    QQC2.TextArea {
        id: area

        objectName: "noteArea"
        wrapMode: TextEdit.Wrap
        Accessible.name: qsTr("Note")
    }
}
