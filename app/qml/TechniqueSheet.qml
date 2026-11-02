import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Technique sheet (clickdummy "Programm", row tapped): name, attack, kanji, how well it sits
// (rate 1–4), the last note, the way on to the video, the notes and the comment, and the
// positions of a sequence (the Kamae exercise of Shinki Toho).
Kirigami.Dialog {
    id: sheet

    property var entry: ({})
    property bool playable: false
    readonly property bool hasVideo: playable && !!entry.video
    readonly property bool hasComment: !!entry.comment
    readonly property var steps: entry.steps || []
    // The dialog's scrolling content (Kirigami puts the column into a ScrollView).
    readonly property Flickable flickable: ((contentItem as QQC2.ScrollView)?.contentItem as Flickable) ?? null

    signal playRequested(var entry)
    signal commentRequested(var entry)
    signal notesRequested(var entry)
    readonly property var lastNote: Practice.notesRevision >= 0 && entry.id ? Practice.lastNote(entry.id) : ({})

    title: entry.title || ""
    preferredWidth: Kirigami.Units.gridUnit * 24
    // Centred without Kirigami's move on opening (y follows the opacity): with the Kamae exercise
    // the sheet grows while it fades in, and y ran into a binding loop.
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: Kirigami.Units.largeSpacing

    // Not a content child: the dialog's scroll view sizes itself by its only child, the column.
    property Item kanteScope: KanteScope { target: sheet.contentItem }

    // Up/Down, Page Up/Down, Home and End scroll a sheet taller than the window (the Kamae
    // exercise on a phone); the mouse wheel does already. From the content and from the header
    // (Close may hold the focus).
    function scrollKey(event) {
        const f = sheet.flickable
        const line = Kirigami.Units.gridUnit * 2
        const step = {
            [Qt.Key_Up]: -line, [Qt.Key_Down]: line,
            [Qt.Key_PageUp]: f ? -f.height * 0.9 : 0, [Qt.Key_PageDown]: f ? f.height * 0.9 : 0,
            [Qt.Key_Home]: -Infinity, [Qt.Key_End]: Infinity
        }[event.key]
        if (!f || step === undefined) {
            return
        }
        f.contentY = Math.max(0, Math.min(Math.max(0, f.contentHeight - f.height), f.contentY + step))
        event.accepted = true
    }

    // Kirigami's header, with the keys above.
    header: Kirigami.DialogHeader {
        dialog: sheet
        contentItem: Kirigami.DialogHeaderTopContent {
            dialog: sheet
        }
        Keys.onPressed: event => sheet.scrollKey(event)
    }

    // The popup itself holds the focus when it opens; the column takes it for the keys.
    onOpened: column.forceActiveFocus()

    ColumnLayout {
        id: column

        spacing: Kirigami.Units.smallSpacing
        Keys.onPressed: event => sheet.scrollKey(event)

        QQC2.Label {
            Layout.fillWidth: true
            visible: !!sheet.entry.attack
            text: sheet.entry.attack || ""
            color: KanteStyle.disabledTextColor
            wrapMode: Text.Wrap
        }
        KanteHeading {
            Layout.fillWidth: true
            visible: !!sheet.entry.jp
            level: 2
            text: sheet.entry.jp || ""
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: !!sheet.entry.extra
            text: sheet.entry.extra || ""
            wrapMode: Text.Wrap
        }
        RatingRow {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            entry: sheet.entry.id || ""
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: !!sheet.lastNote.text
            text: qsTr("Last note: %1").arg(sheet.lastNote.text || "")
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        RowLayout {
            Layout.topMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            KanteButton {
                visible: sheet.hasVideo
                emphasis: KanteButton.Emphasis.Primary
                icon.name: "media-playback-start"
                text: qsTr("Watch video")
                onClicked: sheet.playRequested(sheet.entry)
            }
            KanteButton {
                icon.name: "view-pim-notes"
                text: qsTr("Notes")
                onClicked: {
                    sheet.close()
                    sheet.notesRequested(sheet.entry)
                }
            }
            KanteButton {
                visible: sheet.hasComment
                icon.name: "documentinfo"
                text: qsTr("Comment")
                onClicked: {
                    sheet.close()
                    sheet.commentRequested(sheet.entry)
                }
            }
        }

        // The description of a sequence (Kamae exercise), after the rating and the buttons, so
        // those stay in view.
        StepList {
            objectName: "sheetSteps"
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            steps: sheet.steps
        }
    }
}
