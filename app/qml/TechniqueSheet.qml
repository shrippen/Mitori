import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
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
    // Without the video on the device: to the videos and DVD codes.
    signal videosRequested()
    readonly property var lastNote: Practice.notesRevision >= 0 && entry.id ? Practice.lastNote(entry.id) : ({})

    // The form's number before the name, as in the list ("3 · Kote-gaeshi").
    title: (entry.nr ? entry.nr + " · " : "") + (entry.title || "")
    readonly property int noteCount: Practice.notesRevision >= 0 && entry.id ? Practice.notes(entry.id).length : 0
    preferredWidth: Kirigami.Units.gridUnit * 24
    // Under Shinki the sheet is a sheet from the bottom, the whole width (the clickdummy); under
    // the other styles it stays the centred dialog.
    readonly property bool shinkiSheet: Look.shinki
    width: shinkiSheet && parent ? parent.width : Math.min(preferredWidth, absoluteMaximumWidth)
    // Centred without Kirigami's move on opening (y follows the opacity): with the Kamae exercise
    // the sheet grows while it fades in, and y ran into a binding loop.
    // From the bottom it grows with its content (a new last note); Kirigami's height stays as it opened.
    height: shinkiSheet && flickable ? Math.min(flickable.contentHeight + topPadding + bottomPadding + 2 * Look.borderWidth,
                                                maximumHeight)
                                     : implicitHeight
    y: parent ? (shinkiSheet ? Math.round(parent.height - height) : Math.round((parent.height - height) / 2)) : 0
    padding: shinkiSheet ? Look.unit(16) : Kirigami.Units.largeSpacing

    // The panel: the style's ground, its frame, no rounding (the clickdummy's sheet).
    background: Rectangle {
        color: Look.backgroundColor
        border.width: Look.borderWidth
        border.color: Look.ruleColor
    }

    // Not a content child: the dialog's scroll view sizes itself by its only child, the column.
    property Item lookScope: LookScope { target: sheet.contentItem }

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

    // The header: Kirigami's, with the keys above; under Shinki the sheet's own (a grip, the name
    // in the book face, its Japanese below it).
    header: Kirigami.DialogHeader {
        dialog: sheet
        // Under Shinki the sheet has its own head (in the content): the grip, the name in the book
        // face, its Japanese.
        visible: !sheet.shinkiSheet
        height: sheet.shinkiSheet ? 0 : implicitHeight
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

        // The head of the sheet under Shinki (the clickdummy): the grip, the name in the book face,
        // its Japanese below it.
        Item {
            objectName: "shinkiSheetHead"

            Layout.fillWidth: true
            Layout.preferredHeight: sheetHead.implicitHeight + Look.unit(16)
            visible: sheet.shinkiSheet

            // The keys above (Page Down, Home, End) scroll the sheet, as in Kirigami's header.
            Keys.onPressed: event => sheet.scrollKey(event)

            // The grip: the sheet is pulled up from the bottom.
            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: Look.unit(40)
                height: Look.unit(4)
                color: Look.ruleColor
            }

            ColumnLayout {
                id: sheetHead

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Look.unit(8)
                spacing: 0

                LookLabel {
                    Layout.fillWidth: true
                    text: sheet.title
                    font: Look.sheetTitleFont
                    wrapMode: Text.Wrap
                }
                LookLabel {
                    Layout.fillWidth: true
                    visible: !!sheet.entry.jp
                    text: sheet.entry.jp || ""
                    font: Look.japaneseFont(Look.defaultFont)
                    color: Look.mutedTextColor
                }
            }

            LookToolButton {
                objectName: "sheetClose"
                anchors.top: parent.top
                anchors.right: parent.right
                icon.name: "window-close"
                text: qsTr("Close")
                display: QQC2.AbstractButton.IconOnly
                onClicked: sheet.close()
            }
        }

        // Where it stands: attack, its addition, grade ("Shomen-uchi · Irimi · 5. Kyu").
        LookLabel {
            objectName: "sheetEyebrow"
            Layout.fillWidth: true
            text: [sheet.entry.attack, sheet.entry.attackExtra, sheet.entry.gradeName].filter(x => !!x).join(" · ")
            visible: text !== ""
            color: Look.disabledTextColor
            wrapMode: Text.Wrap
        }
        LookHeading {
            Layout.fillWidth: true
            visible: !sheet.shinkiSheet && !!sheet.entry.jp      // Shinki's head has it
            level: 2
            text: sheet.entry.jp || ""
        }
        LookLabel {
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
        LookLabel {
            objectName: "sheetLastNote"
            Layout.fillWidth: true
            // A photo or voice note without text is named as such.
            readonly property string shown: (sheet.lastNote.text || (Media.kind(sheet.lastNote.media || "") === Media.Kind.Photo ? qsTr("Photo")
                               : Media.kind(sheet.lastNote.media || "") === Media.Kind.Voice ? qsTr("Voice note") : ""))
            visible: shown !== ""
            text: shown === "" ? "" : qsTr("Last note, %1: %2").arg(sheet.lastNote.created.toLocaleDateString(Qt.locale(), Locale.ShortFormat)).arg(shown)
            color: Look.mutedTextColor
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        // Without the video: no video for this grade at all, or one to load with the DVD's code.
        LookHint {
            objectName: "sheetNoVideo"
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            visible: !sheet.hasVideo
            titleText: sheet.entry.video ? qsTr("Video not loaded. The DVD's code loads it onto the device.")
                                    : qsTr("There is no video of this technique.")

            LookButton {
                objectName: "sheetCodeButton"
                visible: !!sheet.entry.video
                text: qsTr("Enter code")
                onClicked: {
                    sheet.close()
                    sheet.videosRequested()
                }
            }
        }
        RowLayout {
            Layout.topMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            LookButton {
                visible: sheet.hasVideo
                role: LookButton.Role.Primary
                icon.name: "media-playback-start"
                text: qsTr("Watch video")
                onClicked: sheet.playRequested(sheet.entry)
            }
            LookButton {
                objectName: "sheetNotesButton"
                icon.name: "view-pim-notes"
                text: sheet.noteCount > 0 ? qsTr("Notes (%1)").arg(sheet.noteCount) : qsTr("Notes")
                onClicked: {
                    sheet.close()
                    sheet.notesRequested(sheet.entry)
                }
            }
            LookButton {
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
