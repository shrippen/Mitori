pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Date field (bible section 5, Datumsfeld): the field of section 5 with a calendar button; the
// button opens the month on a card: ‹ month year ›, seven columns, the chosen day red filled,
// today framed in ink, "Today" as a link.
//   typed   "1.10.2026", "1.10.26", "1.10." (this year), "2026-10-01"; Enter or leaving the
//           field takes it, an unreadable date marks the field `invalid`
// `date` is a JS Date or null; `dateEdited(date)` fires on every change by the user. Same
// interface as KanteDateField (date, format, placeholderText, dayNames, todayText, open()).
FocusScope {
    id: root

    property var date: null
    property string format: "dd.MM.yyyy"
    property string placeholderText: format.toUpperCase()
    property var dayNames: ["MO", "DI", "MI", "DO", "FR", "SA", "SO"]
    property string todayText: "Today"
    property var popupAbove: undefined
    signal dateEdited(var date)

    // The text is no date (a page locks its button on it), as KanteDateField.
    readonly property bool invalid: input.invalid
    readonly property alias popupOpen: monthPopup.visible

    // Years a two-digit year may lie ahead of today: "26" is 2026, "99" is 1999.
    readonly property int twoDigitYearsAhead: 20

    implicitWidth: ShinkiStyle.unit(170)
    implicitHeight: input.implicitHeight

    // Full year of a two-digit year, in the century around today.
    function fullYear(twoDigits) {
        const year = new Date().getFullYear()
        let full = Math.floor(year / 100) * 100 + (twoDigits % 100)
        if (full > year + twoDigitYearsAhead) {
            full -= 100
        }
        return full
    }

    // Text -> Date, null if empty, undefined if unreadable.
    function parse(text) {
        const t = String(text).trim()
        if (t === "") {
            return null
        }
        const fixed = Date.fromLocaleString(Qt.locale(), t, format)
        if (fixed instanceof Date && !isNaN(fixed.getTime())) {
            if (/(^|[^y])yy([^y]|$)/.test(format)) {
                fixed.setFullYear(fullYear(fixed.getFullYear()))
            }
            return fixed
        }
        let y, m, d
        const iso = t.match(/^(\d{4})-(\d{1,2})-(\d{1,2})$/)
        const dmy = t.match(/^(\d{1,2})[.\/](\d{1,2})[.\/]?(\d{2}|\d{4})?$/)
        if (iso) {
            y = +iso[1]; m = +iso[2]; d = +iso[3]
        } else if (dmy) {
            d = +dmy[1]; m = +dmy[2]
            y = dmy[3] === undefined ? new Date().getFullYear() : +dmy[3]
            if (y < 100) {
                y = fullYear(y)
            }
        } else {
            return undefined
        }
        const out = new Date(y, m - 1, d)
        // 31.02. rolls over to March: not a date.
        if (out.getFullYear() !== y || out.getMonth() !== m - 1 || out.getDate() !== d) {
            return undefined
        }
        return out
    }

    function open() {
        monthPopup.open()
    }
    function close() {
        monthPopup.close()
    }
    function textOf(date) {
        return date ? Qt.formatDate(date, format) : ""
    }

    // A date from the user: set it, show it, tell the app.
    function pick(d) {
        date = d
        input.invalid = false
        input.text = textOf(d)
        dateEdited(d)
    }

    function commit() {
        const d = parse(input.text)
        if (d === undefined) {
            input.invalid = true
            return
        }
        const same = (d === null && root.date === null)
            || (d !== null && root.date !== null && d.getTime() === new Date(root.date).getTime())
        input.invalid = false
        input.text = textOf(d)
        if (!same) {
            pick(d)
        }
    }

    onDateChanged: {
        if (!input.activeFocus) {
            input.text = textOf(date)
            input.invalid = false
        }
    }

    ShinkiTextField {
        id: input

        anchors.fill: parent
        rightPadding: button.width + ShinkiStyle.gridSpace
        focus: true
        text: root.textOf(root.date)
        // Only while empty: Material floats a placeholder over a filled field.
        placeholderText: text === "" ? root.placeholderText : ""
        inputMethodHints: Qt.ImhDate | Qt.ImhNoPredictiveText
        font.family: ShinkiStyle.uiFamily
        font.features: { "tnum": 1 }
        onAccepted: root.commit()
        onActiveFocusChanged: {
            if (!activeFocus) {
                root.commit()
            }
        }
        Keys.onDownPressed: monthPopup.open()
        // The field keeps the focus, so the popup never sees Escape itself.
        Keys.onEscapePressed: event => {
            event.accepted = monthPopup.visible
            monthPopup.close()
        }
    }
    ShinkiToolButton {
        id: button

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        icon.name: "view-calendar"
        text: root.placeholderText
        display: QQC2.AbstractButton.IconOnly
        focusPolicy: Qt.NoFocus
        onClicked: monthPopup.visible ? monthPopup.close() : monthPopup.open()
    }

    QQC2.Popup {
        id: monthPopup

        objectName: "popup"

        property int year: new Date().getFullYear()
        property int month: new Date().getMonth() + 1
        readonly property int offset: (new Date(year, month - 1, 1).getDay() + 6) % 7
        readonly property int days: new Date(year, month, 0).getDate()
        readonly property var chosen: root.date ? new Date(root.date) : null
        readonly property var now: new Date()

        // Below the field, above it when there is more room there (or popupAbove says so), and
        // inside the window sideways; worked out on every opening (the page may have scrolled).
        function place() {
            const gap = ShinkiStyle.gridSpace
            let top = root
            while (top.parent) {
                top = top.parent
            }
            const p = root.mapToItem(top, 0, 0)
            const h = implicitHeight
            const below = top.height - p.y - root.height - 2 * gap
            const aboveRoom = p.y - 2 * gap
            const up = root.popupAbove === true || root.popupAbove === false ? root.popupAbove
                                                                            : h > below && aboveRoom > below
            x = Math.max(gap - p.x, Math.min(0, top.width - gap - width - p.x))
            y = up ? -Math.min(h, Math.max(0, aboveRoom)) - gap : root.height + gap
        }
        width: 7 * ShinkiStyle.chipMinSpace + 2 * padding
        padding: ShinkiStyle.cardSpace
        closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutsideParent

        onAboutToShow: {
            place()
            const d = new Date(root.parse(input.text) || root.date || new Date())
            year = d.getFullYear()
            month = d.getMonth() + 1
        }
        function step(delta) {
            const m = month - 1 + delta
            year += Math.floor(m / 12)
            month = ((m % 12) + 12) % 12 + 1
        }

        background: Rectangle {
            color: ShinkiStyle.paper
            border.width: ShinkiStyle.lineWidth
            border.color: ShinkiStyle.line
        }

        contentItem: ColumnLayout {
            spacing: ShinkiStyle.gridSpace * 2

            RowLayout {
                Layout.fillWidth: true

                ShinkiToolButton {
                    icon.name: "go-previous"
                    text: "‹"
                    display: QQC2.AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus
                    Accessible.name: "previous month"
                    onClicked: monthPopup.step(-1)
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: Qt.locale().standaloneMonthName(monthPopup.month - 1) + " " + monthPopup.year
                    color: ShinkiStyle.ink
                    font.family: ShinkiStyle.uiFamily
                    font.bold: true
                    font.pixelSize: ShinkiStyle.uiSize
                }
                ShinkiToolButton {
                    icon.name: "go-next"
                    text: "›"
                    display: QQC2.AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus
                    Accessible.name: "next month"
                    onClicked: monthPopup.step(1)
                }
            }

            // Seven columns, Monday first; the day names as a column.
            Grid {
                objectName: "monthGrid"
                columns: 7

                Repeater {
                    model: root.dayNames

                    QQC2.Label {
                        required property var modelData

                        width: ShinkiStyle.chipMinSpace
                        horizontalAlignment: Text.AlignHCenter
                        text: modelData
                        color: ShinkiStyle.inkMuted
                        font.family: ShinkiStyle.narrowFamily
                        font.bold: true
                        font.pixelSize: ShinkiStyle.columnSize
                    }
                }
                Repeater {
                    model: monthPopup.offset

                    Item {
                        width: ShinkiStyle.chipMinSpace
                        height: ShinkiStyle.chipMinSpace
                    }
                }
                Repeater {
                    model: monthPopup.days

                    QQC2.AbstractButton {
                        id: cell

                        required property int index
                        readonly property int day: index + 1
                        readonly property bool chosen: !!monthPopup.chosen && monthPopup.chosen.getFullYear() === monthPopup.year
                            && monthPopup.chosen.getMonth() + 1 === monthPopup.month && monthPopup.chosen.getDate() === day
                        readonly property bool today: monthPopup.now.getFullYear() === monthPopup.year
                            && monthPopup.now.getMonth() + 1 === monthPopup.month && monthPopup.now.getDate() === day

                        width: ShinkiStyle.chipMinSpace
                        height: ShinkiStyle.chipMinSpace
                        focusPolicy: Qt.NoFocus
                        text: String(day)
                        Accessible.checked: chosen
                        onClicked: {
                            root.pick(new Date(monthPopup.year, monthPopup.month - 1, day))
                            monthPopup.close()
                        }

                        background: Rectangle {
                            color: cell.chosen ? ShinkiStyle.red : cell.hovered ? ShinkiStyle.chip : "transparent"
                            border.width: cell.today && !cell.chosen ? ShinkiStyle.lineWidth : 0
                            border.color: ShinkiStyle.ink
                        }
                        contentItem: QQC2.Label {
                            text: cell.text
                            color: cell.chosen ? ShinkiStyle.textOnRed : ShinkiStyle.ink
                            font.family: ShinkiStyle.uiFamily
                            font.bold: cell.chosen
                            font.pixelSize: ShinkiStyle.uiSize
                            font.features: { "tnum": 1 }
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            ShinkiButton {
                Layout.alignment: Qt.AlignRight
                link: true
                text: root.todayText
                focusPolicy: Qt.NoFocus
                onClicked: {
                    const t = new Date()
                    root.pick(new Date(t.getFullYear(), t.getMonth(), t.getDate()))
                    monthPopup.close()
                }
            }
        }
    }
}
