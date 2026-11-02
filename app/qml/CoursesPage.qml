pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Courses (clickdummy "Lehrgänge"): the coming dates of the association's course calendar and
// the user's notebooks. A date opens its notebook (made on first use); "New course" makes one
// without a calendar event (a private training, a guest teacher).
//
//   NEXT DATES · course calendar        loaded from the network, kept for offline use
//   [ 6 ] Daitoryu Aiki Jujutsu         place · 6.–7.6.2026 · teacher       in 12 days
//    Jun                                                                     3 notes
//   MY NOTEBOOKS                         every notebook, the latest course first
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    readonly property var upcoming: Courses.upcoming
    readonly property var notebooks: Courses.revision >= 0 ? Courses.notebooks : []

    title: qsTr("Courses")

    actions: [
        Kirigami.Action {
            objectName: "newCourse"
            icon.name: "list-add"
            text: qsTr("New course")
            onTriggered: page.Kirigami.PageStack.push(Qt.resolvedUrl("CourseNewPage.qml"))
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
        if (notebook) {
            page.Kirigami.PageStack.push(Qt.resolvedUrl("CoursePage.qml"), { uid: notebook })
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            Layout.fillWidth: true
            visible: Courses.errorString !== ""
            text: Courses.errorString
            color: KanteStyle.negativeTextColor
            wrapMode: Text.Wrap
        }

        KanteSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("Next dates")
        }
        QQC2.Label {
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
            color: Courses.calendarState === Courses.CalendarState.Failed ? KanteStyle.neutralTextColor : KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }

        Repeater {
            model: page.upcoming

            CourseRow {
                required property var modelData

                objectName: "eventRow"
                Layout.fillWidth: true
                course: modelData
                onClicked: page.open(modelData.notebook || Courses.notebookFor(modelData.uid))
            }
        }

        KanteSectionLabel {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            rule: true
            text: qsTr("My notebooks")
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: page.notebooks.length === 0
            text: qsTr("No notebooks yet. Tap a date above, or make a course of your own (+).")
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }
        Repeater {
            model: page.notebooks

            CourseRow {
                required property var modelData

                objectName: "notebookRow"
                Layout.fillWidth: true
                course: modelData
                onClicked: page.open(modelData.uid)
            }
        }
    }
}
