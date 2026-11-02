.pragma library

.import "coursedays.js" as CourseDays

// Texts of Courses.waiting() and Courses.nextExam(), shared by the pages that show them. The
// Wait states as numbers: the Courses type is not known in a JS library (as Courses.Wait).
const kFirst = 0
const kFromStart = 1
const kUnknown = 2
const kEarliest = 3
const kFits = 4
const kTooEarly = 5

function shortDate(iso) {
    return CourseDays.day(iso).toLocaleDateString(Qt.locale(), Qt.locale().dateFormat(CourseDays.kShortFormat))
}

// "from 08.03.26", "fits (from 08.03.26)", "too early: from 08.03.26",
// "1 year after 5. Kyu: enter that grade", "2 years after the start of training: enter it",
// "first grade".
function waitText(w) {
    switch (w.state) {
    case kFirst:
        return qsTr("first grade, no waiting time")
    case kFromStart:
        return qsTr("%n year(s) after the start of training: enter it", "", w.years)
    case kUnknown:
        return qsTr("%n year(s) after %1: enter that grade", "", w.years).arg(w.previous)
    case kEarliest:
        return qsTr("from %1").arg(shortDate(w.earliest))
    case kFits:
        return qsTr("fits (from %1)").arg(shortDate(w.earliest))
    case kTooEarly:
        return qsTr("too early: from %1").arg(shortDate(w.earliest))
    default:
        return ""
    }
}

// Whether a waiting time stands against the planned day (a warning colour).
function tooEarly(w) {
    return w.state === kTooEarly
}

// "Exam Aikido 4. Kyu in 30 days", "… today".
function examText(e) {
    return e.days > 0 ? qsTr("Exam %1 in %n day(s)", "", e.days).arg(e.name) : qsTr("Exam %1 today").arg(e.name)
}
