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

        LookLabel {
            Layout.fillWidth: true
            text: qsTr("For private trainings, guest teachers or dates that are not in the calendar.")
            color: Look.mutedTextColor
            wrapMode: Text.Wrap
        }

        // The fields as rows of their own: the name of the field as a small caps line above it.
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Look.unit(16)

            LookFormRow {
                label: qsTr("Title")
                LookTextField {
                    id: titleField

                    objectName: "courseTitleField"
                    Layout.fillWidth: true
                    placeholderText: qsTr("Training with a guest teacher")
                }
            }
            LookFormRow {
                label: qsTr("From")
                LookDateField {
                    id: firstField

                    objectName: "courseFirst"
                    format: qsTr("yyyy-MM-dd", "date format")
                    dayNames: [qsTr("MO"), qsTr("TU"), qsTr("WE"), qsTr("TH"), qsTr("FR"), qsTr("SA"), qsTr("SU")]
                    todayText: qsTr("Today")
                    date: new Date()
                }
            }
            LookFormRow {
                label: qsTr("To")
                LookDateField {
                    id: lastField

                    objectName: "courseLast"
                    format: firstField.format
                    dayNames: firstField.dayNames
                    todayText: firstField.todayText
                    date: new Date()
                }
            }
            LookFormRow {
                label: qsTr("Place")
                LookTextField {
                    id: placeField

                    objectName: "coursePlace"
                    Layout.fillWidth: true
                    placeholderText: qsTr("Dojo, town")
                }
            }
            LookFormRow {
                label: qsTr("Teacher")
                LookTextField {
                    id: teacherField

                    objectName: "courseTeacher"
                    Layout.fillWidth: true
                    placeholderText: qsTr("Name")
                    onAccepted: page.make()
                }
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
