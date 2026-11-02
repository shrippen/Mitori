pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori
import "coursedays.js" as CourseDays

// A course's notebook (clickdummy "Lehrgang"): the course (days, place, teacher), "In the
// calendar" and the official announcement where the calendar names one, then the entries by
// day; a field at the foot adds one. Press and hold an entry (or Enter on it) to change or
// delete it. Notes stay on the device; export comes with photos and voice notes.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    required property string uid
    readonly property var course: Courses.revision >= 0 ? Courses.notebook(uid) : ({})
    readonly property var entries: Courses.revision >= 0 ? Courses.entries(uid) : []
    // [{key, day: Date, entries: [...]}] in the order written (a course's days run forward).
    readonly property var days: {
        const result = []
        for (const e of entries) {
            const key = e.created.toDateString()
            let day = result.find(d => d.key === key)
            if (!day) {
                day = { key: key, day: e.created, entries: [] }
                result.push(day)
            }
            day.entries.push(e)
        }
        return result
    }

    title: qsTr("Notebook")

    actions: [
        Kirigami.Action {
            objectName: "deleteCourse"
            icon.name: "edit-delete"
            text: qsTr("Delete notebook")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: removeDialog.open()
        }
    ]

    function add() {
        if (Courses.addEntry(page.uid, compose.text)) {
            compose.clear()
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        // The course on a card.
        QQC2.Control {
            Layout.fillWidth: true
            padding: Kirigami.Units.largeSpacing
            background: KanteCard {}

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                KanteSectionLabel {
                    Layout.fillWidth: true
                    text: CourseDays.range(page.course.first, page.course.last) + " · "
                          + (page.course.calendarUid ? qsTr("from the calendar") : qsTr("own entry"))
                }
                KanteHeading {
                    objectName: "courseTitle"
                    Layout.fillWidth: true
                    level: 2
                    text: page.course.title || ""
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: [page.course.place, page.course.teacher].filter(x => x).join(" · ")
                    wrapMode: Text.Wrap
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    KanteButton {
                        objectName: "toCalendar"
                        icon.name: "view-calendar"
                        text: qsTr("In the calendar")
                        onClicked: {
                            if (!Courses.addToDeviceCalendar(page.uid)) {
                                calendarFailed.visible = true
                            }
                        }
                    }
                    KanteButton {
                        objectName: "announcement"
                        visible: !!page.course.announcement
                        icon.name: "internet-services"
                        text: qsTr("Announcement")
                        onClicked: Qt.openUrlExternally(page.course.announcement)
                    }
                }
                QQC2.Label {
                    id: calendarFailed

                    Layout.fillWidth: true
                    visible: false
                    text: qsTr("No calendar app could be opened.")
                    color: KanteStyle.neutralTextColor
                    wrapMode: Text.Wrap
                }
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: Courses.errorString !== ""
            text: Courses.errorString
            color: KanteStyle.negativeTextColor
            wrapMode: Text.Wrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: page.entries.length === 0
            text: qsTr("No entries yet. Write below what was shown.")
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
                    text: dayBlock.modelData.day.toLocaleDateString(Qt.locale(), "dddd, " + Qt.locale().dateFormat(Locale.ShortFormat))
                }
                Repeater {
                    model: dayBlock.modelData.entries

                    QQC2.Label {
                        id: entryItem

                        required property var modelData

                        objectName: "courseEntry"
                        Layout.fillWidth: true
                        text: modelData.created.toLocaleTimeString(Qt.locale(), Locale.ShortFormat) + "  " + modelData.text
                        wrapMode: Text.Wrap
                        padding: Kirigami.Units.smallSpacing
                        activeFocusOnTab: true
                        background: Rectangle {
                            color: KanteStyle.cardColor
                            border.width: entryItem.activeFocus ? KanteStyle.borderWidth : 0
                            border.color: KanteStyle.focusColor
                        }
                        Accessible.role: Accessible.Button
                        Accessible.name: modelData.text
                        Accessible.description: qsTr("Change or delete")
                        Accessible.onPressAction: editor.edit(modelData)
                        Keys.onReturnPressed: editor.edit(modelData)
                        Keys.onEnterPressed: editor.edit(modelData)

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            onLongPressed: editor.edit(entryItem.modelData)
                        }
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: editor.edit(entryItem.modelData)
                        }
                    }
                }
            }
        }
    }

    footer: QQC2.ToolBar {
        RowLayout {
            anchors.fill: parent

            KanteTextField {
                id: compose

                objectName: "courseCompose"
                Layout.fillWidth: true
                placeholderText: qsTr("What was shown?")
                Accessible.name: qsTr("New entry")
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

        title: qsTr("Entry")
        save: (uid, text) => Courses.changeEntry(uid, text)
        remove: uid => Courses.deleteEntry(uid)
    }

    KanteDialog {
        id: removeDialog

        objectName: "removeCourse"
        modal: true
        destructive: true
        size: KanteDialog.Size.Small
        title: qsTr("Delete notebook?")
        acceptText: qsTr("Delete")
        standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
        onAccepted: {
            if (Courses.remove(page.uid)) {
                page.Kirigami.PageStack.pop()
            }
        }

        QQC2.Label {
            width: parent.width
            text: qsTr("The notebook and its %n entry(s) go from this device.", "", page.entries.length)
            wrapMode: Text.Wrap
        }
    }
}
