.pragma library

// Locale.ShortFormat: the Locale type is not known in a JS library.
const kShortFormat = 1

// Days of courses as Courses gives them ("YYYY-MM-DD"), read as local days.
// "2026-06-06" → that day at noon, local time (no time zone moves it to the day before).
function day(iso) {
    if (!iso) {
        return null
    }
    const d = String(iso).split("-").map(Number)
    return new Date(d[0], d[1] - 1, d[2], 12)
}
function iso(date) {
    const p = n => (n < 10 ? "0" : "") + n
    return date.getFullYear() + "-" + p(date.getMonth() + 1) + "-" + p(date.getDate())
}
// Whole days from today to the first day (negative: begun).
function fromToday(first) {
    const now = new Date()
    const today = new Date(now.getFullYear(), now.getMonth(), now.getDate(), 12)
    return Math.round((day(first) - today) / 86400000)
}
// "06.06.26" or "06.06.26 – 07.06.26" (the locale's short dates).
function range(first, last) {
    const a = day(first)
    if (!a) {
        return ""
    }
    const short = Qt.locale().dateFormat(kShortFormat)
    const text = a.toLocaleDateString(Qt.locale(), short)
    return !last || last === first ? text : text + " – " + day(last).toLocaleDateString(Qt.locale(), short)
}
