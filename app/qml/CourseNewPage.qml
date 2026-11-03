import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori
import "coursedays.js" as CourseDays

// A course without a calendar event (clickdummy "Neuer Lehrgang"): a private training, a guest
// teacher, a date not in the calendar. "Make" opens its notebook in place of this page.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    readonly property bool complete: titleField.text.trim() !== "" && !!firstField.date && !firstField.invalid
                                     && !lastField.invalid

    title: qsTr("New course")

    function make() {
        if (!page.complete) {
            return
        }
        const last = lastField.date && lastField.date >= firstField.date ? lastField.date : firstField.date
        const uid = Courses.create(titleField.text, CourseDays.iso(firstField.date), CourseDays.iso(last),
                                   placeField.text, teacherField.text)
        if (uid) {
            page.Kirigami.PageStack.replace(Qt.resolvedUrl("CoursePage.qml"), { uid: uid })
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("For private trainings, guest teachers or dates that are not in the calendar.")
            color: Look.mutedTextColor
            wrapMode: Text.Wrap
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            LookTextField {
                id: titleField

                objectName: "courseTitleField"
                Kirigami.FormData.label: qsTr("Title")
                Layout.fillWidth: true
                placeholderText: qsTr("Training with a guest teacher")
            }
            LookDateField {
                id: firstField

                objectName: "courseFirst"
                Kirigami.FormData.label: qsTr("From")
                format: qsTr("yyyy-MM-dd", "date format")
                dayNames: [qsTr("MO"), qsTr("TU"), qsTr("WE"), qsTr("TH"), qsTr("FR"), qsTr("SA"), qsTr("SU")]
                todayText: qsTr("Today")
                date: new Date()
            }
            LookDateField {
                id: lastField

                objectName: "courseLast"
                Kirigami.FormData.label: qsTr("To")
                format: firstField.format
                dayNames: firstField.dayNames
                todayText: firstField.todayText
                date: new Date()
            }
            LookTextField {
                id: placeField

                objectName: "coursePlace"
                Kirigami.FormData.label: qsTr("Place")
                Layout.fillWidth: true
                placeholderText: qsTr("Dojo, town")
            }
            LookTextField {
                id: teacherField

                objectName: "courseTeacher"
                Kirigami.FormData.label: qsTr("Teacher")
                Layout.fillWidth: true
                placeholderText: qsTr("Name")
                onAccepted: page.make()
            }
        }

        LookButton {
            objectName: "makeCourse"
            role: LookButton.Role.Primary
            enabled: page.complete
            text: qsTr("Make")
            onClicked: page.make()
        }
    }
}
