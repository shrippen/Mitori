pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Notes on a technique (clickdummy "Notizen"): by day, newest day first; a field at the foot
// adds a note for today. Press and hold a note (or right-click) to change or delete it.
// Photos and voice notes come with the platform layer (ROADMAP.md).
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    required property var entry
    // Opened from "All notes": its action goes back there instead of stacking another list.
    property bool fromAllNotes: false
    readonly property var notes: Practice.notesRevision >= 0 ? Practice.notes(entry.id) : []
    // [{day: Date, notes: [...]}], newest day first; within a day in the order written.
    readonly property var days: {
        const result = []
        for (const n of notes) {
            const key = n.created.toDateString()
            let day = result.find(d => d.key === key)
            if (!day) {
                day = { key: key, day: n.created, notes: [] }
                result.push(day)
            }
            day.notes.push(n)
        }
        return result.reverse()
    }

    title: qsTr("Notes")

    actions: [
        Kirigami.Action {
            icon.name: "view-list-details"
            text: qsTr("All notes")
            onTriggered: page.fromAllNotes ? page.Kirigami.PageStack.pop()
                                           : page.Kirigami.PageStack.push(Qt.resolvedUrl("AllNotesPage.qml"))
        }
    ]

    // Practice.today only makes the names follow a new day: as a QDate it reaches JS as
    // midnight UTC, the day before west of UTC.
    function dayName(day) {
        if (Practice.today && day.toDateString() === new Date().toDateString()) {
            return qsTr("Today")
        }
        return day.toLocaleDateString(Qt.locale(), Locale.ShortFormat)
    }

    function add() {
        if (Practice.addNote(page.entry.id, compose.text)) {
            compose.clear()
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                QQC2.Label {
                    Layout.fillWidth: true
                    text: (page.entry.nr ? page.entry.nr + " · " : "") + (page.entry.title || "")
                    font.bold: true
                    wrapMode: Text.Wrap
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: [page.entry.attack, page.entry.jp].filter(x => x).join(" · ")
                    color: KanteStyle.mutedTextColor
                    wrapMode: Text.Wrap
                }
            }
            KanteLevelDots {
                value: Practice.revision >= 0 ? Practice.level(page.entry.id) : 0
                names: Practice.levelNames
                color: KanteStyle.levelColor(value)
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: Practice.errorString !== ""
            text: Practice.errorString
            color: KanteStyle.negativeTextColor
            wrapMode: Text.Wrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: page.notes.length === 0
            text: qsTr("No notes on this technique yet.")
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }

        Repeater {
            model: page.days

            ColumnLayout {
                id: dayBlock

                required property var modelData

                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                KanteSectionLabel {
                    objectName: "dayName"
                    text: page.dayName(dayBlock.modelData.day)
                }
                Repeater {
                    model: dayBlock.modelData.notes

                    QQC2.Label {
                        id: noteItem

                        required property var modelData

                        objectName: "note"
                        Layout.fillWidth: true
                        text: modelData.text
                        wrapMode: Text.Wrap
                        padding: Kirigami.Units.smallSpacing
                        // Keyboard and screen readers: Tab to the note, Enter (or the press action)
                        // opens the editor.
                        activeFocusOnTab: true
                        background: Rectangle {
                            color: KanteStyle.cardColor
                            border.width: noteItem.activeFocus ? KanteStyle.borderWidth : 0
                            border.color: KanteStyle.focusColor
                        }
                        Accessible.role: Accessible.Button
                        Accessible.name: text
                        Accessible.description: qsTr("Change or delete")
                        Accessible.onPressAction: editor.edit(modelData)
                        Keys.onReturnPressed: editor.edit(modelData)
                        Keys.onEnterPressed: editor.edit(modelData)

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            onLongPressed: editor.edit(noteItem.modelData)
                        }
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: editor.edit(noteItem.modelData)
                        }
                    }
                }
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: page.notes.length > 0
            text: qsTr("Press and hold a note (or Enter on it) to change or delete it.")
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }
    }

    footer: QQC2.ToolBar {
        RowLayout {
            anchors.fill: parent

            KanteTextField {
                id: compose

                objectName: "compose"
                Layout.fillWidth: true
                placeholderText: qsTr("Note for today …")
                Accessible.name: qsTr("New note")
                onAccepted: page.add()
            }
            KanteButton {
                emphasis: KanteButton.Emphasis.Primary
                enabled: compose.text.trim() !== ""
                icon.name: "list-add"
                text: qsTr("Add")
                onClicked: page.add()
            }
        }
    }

    NoteEditor {
        id: editor
    }
}
