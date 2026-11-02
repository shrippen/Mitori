pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori
import "coursedays.js" as CourseDays
import "grades.js" as Grades

// A course's notebook (clickdummy "Lehrgang"): the course (days, place, teacher), "In the
// calendar" and the official announcement where the calendar names one, then the entries by
// day; a field at the foot adds one. Press and hold an entry (or Enter on it) to change or
// delete it. Notes stay on the device; export comes with photos and voice notes.
// On a course that has not ended an exam can be planned (discipline and grade), with its
// waiting time checked against the course's first day.
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
        // The notebook as a ZIP (notizbuch.md, photos, voice notes) to a file the user picks.
        Kirigami.Action {
            objectName: "exportCourse"
            icon.name: "document-export"
            text: qsTr("Export")
            onTriggered: {
                exportDialog.selectedFile = exportDialog.currentFolder + "/" + Courses.exportName(page.uid)
                exportDialog.open()
            }
        },
        Kirigami.Action {
            objectName: "deleteCourse"
            icon.name: "edit-delete"
            text: qsTr("Delete notebook")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: removeDialog.open()
        }
    ]

    readonly property bool coming: !!course.last && CourseDays.fromToday(course.last) >= 0
    readonly property bool planned: !!course.examGrade
    // The exam being chosen: the planned one, else the programme's discipline and its next grade.
    property bool choosing: false
    property string examDiscipline: ""
    property string examGrade: ""
    readonly property var examLadder: Courses.revision >= 0 && examDiscipline !== "" ? Courses.ladder(examDiscipline) : []

    function choose() {
        page.examDiscipline = page.course.examDiscipline || Programme.discipline
        page.examGrade = page.course.examGrade || Courses.nextGrade(page.examDiscipline)
        page.choosing = true
    }

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
                    objectName: "exportedTo"
                    Layout.fillWidth: true
                    visible: page.exported !== ""
                    text: qsTr("Exported: %1 (notizbuch.md with the photos and voice notes)").arg(page.exported)
                    color: KanteStyle.positiveTextColor
                    wrapMode: Text.Wrap
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

        // The exam on this course: planned (with its waiting time) or "Plan an exam here …".
        ColumnLayout {
            objectName: "examBlock"
            Layout.fillWidth: true
            visible: page.planned || page.coming
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                visible: page.planned && !page.choosing

                KanteChip {
                    objectName: "examChip"
                    text: qsTr("Exam %1").arg(page.course.examName || "")
                    checkable: true
                    checked: true
                    interactive: false
                }
                QQC2.Label {
                    objectName: "examWait"
                    Layout.fillWidth: true
                    readonly property var wait: Courses.revision >= 0 && page.planned
                                                ? Courses.waiting(page.course.examDiscipline, page.course.examGrade, page.course.first) : ({})
                    text: Grades.waitText(wait)
                    color: Grades.tooEarly(wait) ? KanteStyle.neutralTextColor : KanteStyle.mutedTextColor
                    wrapMode: Text.Wrap
                }
                KanteButton {
                    objectName: "changeExam"
                    visible: page.coming
                    text: qsTr("Change")
                    onClicked: page.choose()
                }
            }
            KanteButton {
                objectName: "planExam"
                visible: !page.planned && page.coming && !page.choosing
                icon.name: "view-calendar"
                text: qsTr("Plan an exam here …")
                onClicked: page.choose()
            }

            // Choosing: discipline, grade (the next one marked), its waiting time.
            QQC2.Control {
                Layout.fillWidth: true
                visible: page.choosing
                padding: Kirigami.Units.largeSpacing
                background: KanteCard {}

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    KanteHeading {
                        Layout.fillWidth: true
                        level: 3
                        text: qsTr("Plan an exam here")
                    }
                    KanteSegmented {
                        Layout.fillWidth: true
                        model: Programme.disciplines.map(d => d.kurzname)
                        currentIndex: Programme.disciplines.findIndex(d => d.id === page.examDiscipline)
                        onActivated: index => {
                            page.examDiscipline = Programme.disciplines[index].id
                            page.examGrade = Courses.nextGrade(page.examDiscipline)
                        }
                        Accessible.name: qsTr("Discipline")
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        Repeater {
                            model: page.examLadder

                            KanteChip {
                                required property var modelData
                                readonly property bool next: modelData.id === Courses.nextGrade(page.examDiscipline)

                                objectName: "examGrade_" + modelData.id
                                text: modelData.name + (next ? " · " + qsTr("next") : modelData.passed ? " · " + qsTr("passed") : "")
                                checkable: true
                                checked: modelData.id === page.examGrade
                                onClicked: page.examGrade = modelData.id
                            }
                        }
                    }
                    QQC2.Label {
                        objectName: "choiceWait"
                        Layout.fillWidth: true
                        readonly property var wait: page.examGrade !== "" ? Courses.waiting(page.examDiscipline, page.examGrade, page.course.first) : ({})
                        text: page.examGrade !== "" ? qsTr("Waiting time: %1").arg(Grades.waitText(wait)) : qsTr("All grades passed.")
                        color: Grades.tooEarly(wait) ? KanteStyle.neutralTextColor : KanteStyle.mutedTextColor
                        wrapMode: Text.Wrap
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        KanteButton {
                            objectName: "examPlan"
                            emphasis: KanteButton.Emphasis.Primary
                            enabled: page.examGrade !== ""
                            text: qsTr("Plan")
                            onClicked: {
                                if (Courses.planExam(page.uid, page.examDiscipline, page.examGrade)) {
                                    page.choosing = false
                                }
                            }
                        }
                        KanteButton {
                            objectName: "examNone"
                            text: qsTr("No exam")
                            onClicked: {
                                if (Courses.planExam(page.uid, "", "")) {
                                    page.choosing = false
                                }
                            }
                        }
                        KanteButton {
                            text: qsTr("Cancel")
                            onClicked: page.choosing = false
                        }
                    }
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

                    ColumnLayout {
                        id: entryItem

                        required property var modelData

                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Label {
                            id: entryItemLabel

                            // The note, also for tests that find the label.
                            readonly property var modelData: entryItem.modelData

                            objectName: "courseEntry"
                            Layout.fillWidth: true
                            text: entryItem.modelData.created.toLocaleTimeString(Qt.locale(), Locale.ShortFormat) + "  " + (entryItem.modelData.text || page.mediaWord(entryItem.modelData.media))
                            wrapMode: Text.Wrap
                            padding: Kirigami.Units.smallSpacing
                            activeFocusOnTab: true
                            background: Rectangle {
                                color: KanteStyle.cardColor
                                border.width: entryItemLabel.activeFocus ? KanteStyle.borderWidth : 0
                                border.color: KanteStyle.focusColor
                            }
                            Accessible.role: Accessible.Button
                            Accessible.name: entryItem.modelData.text || page.mediaWord(entryItem.modelData.media)
                            Accessible.description: qsTr("Change or delete")
                            Accessible.onPressAction: editor.edit(entryItem.modelData)
                            Keys.onReturnPressed: editor.edit(entryItem.modelData)
                            Keys.onEnterPressed: editor.edit(entryItem.modelData)

                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                onLongPressed: editor.edit(entryItem.modelData)
                            }
                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: editor.edit(entryItem.modelData)
                            }
                        }
                        MediaView {
                            Layout.fillWidth: true
                            media: entryItem.modelData.media || ""
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
            // A photo or voice note becomes an entry at once, with the text typed so far.
            MediaCompose {
                objectName: "courseMedia"
                stackPage: page
                onAttached: name => {
                    if (Courses.addEntry(page.uid, compose.text, name)) {
                        compose.clear()
                    } else {
                        Media.discard(name)
                    }
                }
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

    // Where the export went, shown under the course ("" before one).
    property string exported: ""

    FileDialog {
        id: exportDialog

        title: qsTr("Export notebook")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "zip"
        nameFilters: [qsTr("ZIP archives (*.zip)")]
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        onAccepted: page.exportTo(selectedFile)
    }

    function exportTo(url) {
        page.exported = Courses.exportNotebook(page.uid, url) ? decodeURIComponent(String(url).split("/").pop()) : ""
    }

    // A note without text says what it holds.
    function mediaWord(name) {
        const kind = Media.kind(name || "")
        return kind === Media.Kind.Photo ? qsTr("Photo") : kind === Media.Kind.Voice ? qsTr("Voice note") : ""
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
