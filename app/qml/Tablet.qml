pragma Singleton
import QtQuick
import org.kde.kirigami as Kirigami

// The tablet sideways (app-entscheidungen.md, "Tablet quer"), for every page that lays itself out
// for it: the breakpoint, and the width of a page of text and figures (the clickdummy's
// `.tab .page`).
QtObject {
    // A page at least this high: a 600 dp tablet less its bars. A phone held sideways has about 20.
    readonly property real minHeight: Kirigami.Units.gridUnit * 26
    // Nearly square windows are no tablet sideways: two columns would run over the edge.
    readonly property real minWidth: Kirigami.Units.gridUnit * 38
    readonly property real pageWidth: Look.unit(880)

    function wide(width, height) {
        return width > height && height >= minHeight && width >= minWidth
    }
    // Beside a page of `pageWidth` in the middle of `width`, at least `padding`.
    function sidePadding(width, padding) {
        return Math.max(padding, (width - pageWidth) / 2)
    }
}
