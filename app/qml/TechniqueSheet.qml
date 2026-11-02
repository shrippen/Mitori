import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Technique sheet (clickdummy "Programm", row tapped): name, attack, kanji, how well it sits
// (rate 1–4), the last note, and the way on to the video, the notes and the comment.
Kirigami.Dialog {
    id: sheet

    property var entry: ({})
    property bool playable: false
    readonly property bool hasVideo: playable && !!entry.video
    readonly property bool hasComment: !!entry.comment

    signal playRequested(var entry)
    signal commentRequested(var entry)
    signal notesRequested(var entry)
    readonly property var lastNote: Practice.notesRevision >= 0 && entry.id ? Practice.lastNote(entry.id) : ({})

    title: entry.title || ""
    preferredWidth: Kirigami.Units.gridUnit * 24
    padding: Kirigami.Units.largeSpacing

    // Not a content child: the dialog's scroll view sizes itself by its only child, the column.
    property Item kanteScope: KanteScope { target: sheet.contentItem }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

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
    }
}
