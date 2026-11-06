import org.kde.kirigami as Kirigami
import Mitori

// A page with a note field, as the player has it, without a video (tst_look.qml).
Kirigami.Page {
    required property string entryId

    NoteRow {
        entry: ({ id: entryId })
    }
}
