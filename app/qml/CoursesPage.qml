pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori
import "coursedays.js" as CourseDays
import "grades.js" as Grades

// Courses (clickdummy "Lehrgänge"): the coming dates of the association's course calendar. A date
// opens its notebook (made on first use); "New course" makes one without a calendar event (a
// private training, a guest teacher). On top "My grades", "My notebooks" (NotebooksPage, owner
// 06.11.2026) and the exams planned on coming courses, with their waiting times.
//
//   [段 My grades                    ] [記 My notebooks                ]
//   [   Aikido 4. Kyu · Daitoryu 5. Kyu] [   3 notebooks                ]
//   PLANNED EXAMS   Aikido 3. Kyu  Bonn, 12.06.26          fits (from 09.03.26)
//   NEXT DATES · course calendar        loaded from the network, kept for offline use
//   [ 6 ] Daitoryu Aiki Jujutsu         place · 6.–7.6.2026 · teacher       in 12 days
//    Jun                                                                     3 notes
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    readonly property var upcoming: Courses.upcoming
    readonly property var notebooks: Courses.revision >= 0 ? Courses.notebooks : []
    // Exams on courses that have not ended, the next first.
    readonly property var planned: notebooks.filter(n => n.examGrade && CourseDays.fromToday(n.last) >= 0)
                                            .sort((a, b) => a.first.localeCompare(b.first))
    // What fits a tile on a phone in any font (Kante's tile cuts after two lines).
    readonly property int gradesTextLetters: 34
    // The highest grade passed per discipline: "Aikido 4. Kyu · Daitoryu 5. Kyu"; what does not
    // fit is counted: "Aikido 4. Kyu +2".
    readonly property string gradesText: {
        const grades = Courses.revision >= 0 ? Courses.grades : []
        const parts = []
        for (const d of Programme.disciplines) {
            const ladder = Courses.ladder(d.id)
            const passed = ladder.filter(g => g.passed)
            if (passed.length > 0) {
                parts.push(d.kurzname + " " + passed[passed.length - 1].name)
            }
        }
        if (grades.length === 0) {
            return qsTr("none entered yet")
        }

        const whole = parts.join(" · ")
        return whole.length <= gradesTextLetters ? whole : parts[0] + " +" + (parts.length - 1)
    }

    // ListAndPage, when it shows this page: on a tablet sideways a date's notebook stands beside
    // the list, the first one before a date is chosen.
    property var paneHost: null
    readonly property var firstPane: notebooks.length > 0 ? { source: "CoursePage.qml", props: { uid: notebooks[0].uid } } : null

    title: qsTr("Courses")

    actions: [
        Kirigami.Action {
            objectName: "newCourse"
            icon.name: "list-add"
            text: qsTr("New course")
            onTriggered: page.Kirigami.PageStack.push(Qt.resolvedUrl("CourseNewPage.qml"), { paneHost: page.paneHost })
        },
        Kirigami.Action {
            icon.name: "view-refresh"
            text: qsTr("Reload")
            enabled: Courses.calendarState !== Courses.CalendarState.Loading
            onTriggered: Courses.reload()
        }
    ]

    // Opening the page loads the calendar when the last load is a few hours old.
    Component.onCompleted: Courses.reload(Courses.Reload.Older)

    function open(notebook) {
        if (!notebook) {
            return
        }
        if (!page.paneHost || !page.paneHost.show("CoursePage.qml", { uid: notebook })) {
            page.Kirigami.PageStack.push(Qt.resolvedUrl("CoursePage.qml"), { uid: notebook })
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        LookLabel {
            Layout.fillWidth: true
            visible: Courses.errorString !== ""
            text: Courses.errorString
            color: Look.negativeTextColor
            wrapMode: Text.Wrap
        }

        // Side by side at every width: equal columns, as high as the higher one; the tiles wrap.
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing

            LookTile {
                objectName: "myGrades"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1          // equal columns
                glyph: "段"
                text: qsTr("My grades")
                hint: page.gradesText
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("GradesPage.qml"))
            }

            LookTile {
                objectName: "myNotebooks"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1          // equal columns
                glyph: "記"
                text: qsTr("My notebooks")
                hint: qsTr("%n notebook(s)", "", page.notebooks.length)
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("NotebooksPage.qml"))
            }
        }

        LookSectionLabel {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            visible: page.planned.length > 0
            rule: true
            text: qsTr("Planned exams")
        }
        Repeater {
            model: page.planned

            LookListRow {
                required property var modelData
                readonly property var wait: Courses.waiting(modelData.examDiscipline, modelData.examGrade, modelData.first)

                objectName: "plannedRow"
                Layout.fillWidth: true
                text: modelData.examName
                subtitle: (modelData.place || modelData.title) + ", " + Grades.shortDate(modelData.first)
                meta: Grades.waitText(wait)
                selected: !!page.paneHost && page.paneHost.isShown("CoursePage.qml", modelData.uid)
                onClicked: page.open(modelData.uid)
            }
        }

        LookSectionLabel {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            rule: true
            text: qsTr("Next dates")
        }
        LookLabel {
            objectName: "calendarState"
            Layout.fillWidth: true
            text: {
                const loaded = Courses.calendarLoaded
                const when = loaded.getTime() > 0 ? loaded.toLocaleString(Qt.locale(), Locale.ShortFormat) : ""
                switch (Courses.calendarState) {
                case Courses.CalendarState.Loading:
                    return qsTr("Loading the course calendar …")
                case Courses.CalendarState.Failed:
                    return when ? qsTr("The course calendar could not be loaded; as of %1.").arg(when)
                                : qsTr("The course calendar could not be loaded. Check the connection and reload.")
                case Courses.CalendarState.Ready:
                    return page.upcoming.length > 0
                           ? qsTr("From the association's course calendar, as of %1. Tap a date for its notebook.").arg(when)
                           : qsTr("No coming dates in the course calendar.")
                default:
                    return qsTr("The course calendar is not loaded yet.")
                }
            }
            color: Courses.calendarState === Courses.CalendarState.Failed ? Look.neutralTextColor : Look.mutedTextColor
            wrapMode: Text.Wrap
        }

        Repeater {
            model: page.upcoming

            CourseRow {
                required property var modelData

                objectName: "eventRow"
                Layout.fillWidth: true
                course: modelData
                selected: !!page.paneHost && !!modelData.notebook && page.paneHost.isShown("CoursePage.qml", modelData.notebook)
                onClicked: page.open(modelData.notebook || Courses.notebookFor(modelData.uid))
            }
        }
    }
}
