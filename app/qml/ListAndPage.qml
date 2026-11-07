import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// A list page and, on a tablet sideways, the page chosen in it beside it (the clickdummy's `.split2`).
// On a phone only the list; its rows open their pages on the window's stack.
//
//   sideways                          upright, once sideways: same page object
//   ┌ list ─────┬ page ─────────────┐ ┌ list ───┐  row  ┌ page ───┐
//   │ rows …    │ the row's page    │ │ rows …  │  ───► │ …       │
//   └───────────┴───────────────────┘ └─────────┘ ◄Back └─────────┘
//
// A turn only changes what shows: the page beside the list lives on, with its fields, its dialogs
// and the callbacks of a camera or scan page above it.
//
// A list page and a page beside it may have `paneHost`: then they get this page. A list asks
// show(file, props) and pushes the page itself when it answers false; it may name `firstPane`,
// what stands beside it before a row is chosen. A pushed page finds the window's stack as usual.
// A page beside the list may have `busy` and finishBusy(): it finishes before another takes its place.
Kirigami.Page {
    id: host

    required property string listSource
    // The row chosen: { source, props }, null until then.
    property var chosen: null
    // What the list named to stand there before that: it stays when the list's first row changes
    // (a sync brought a notebook), until it goes.
    property var first: null
    readonly property var current: chosen || first
    function takeFirst() {
        if (!first && list.item) {
            first = list.item.firstPane ?? null
        }
    }

    readonly property TypingHold hold: TypingHold { target: host }
    readonly property bool wide: hold.wide
    // Once sideways this is a tablet, and the page stays through every turn. A phone loads none.
    property bool everWide: false
    // Upright the chosen page fills the window, Back leads to the list.
    readonly property bool showsPage: !wide && !!chosen

    // Whether a page is the one beside the list; with `uid` that notebook of it.
    function isShown(source, uid) {
        return wide && !!current && current.source === source && (uid === undefined || current.props.uid === uid)
    }
    function show(source, props) {
        if (!everWide) {
            return false
        }
        choose({ source: source, props: props || {} })
        return true
    }
    // The page beside the list went by itself (its notebook was deleted).
    function clearPane() {
        pending = undefined
        chosen = null
        first = null
        takeFirst()
    }

    // What is to follow a busy page (null: the list); undefined: nothing waits.
    property var pending: undefined
    function choose(next) {
        // Its own row again: a recording runs on.
        if (next && keyOf(next) === keyOf(current)) {
            pending = undefined
            chosen = next
            return
        }
        if (pane.item && pane.item.busy) {
            pending = next
            pane.item.finishBusy()
            return
        }
        chosen = next
    }
    Connections {
        target: pane.item
        ignoreUnknownSignals: true
        function onBusyChanged() {
            if (pane.item.busy || host.pending === undefined) {
                return
            }
            const next = host.pending
            host.pending = undefined
            host.chosen = next
        }
    }

    function keyOf(page) {
        return page ? page.source + JSON.stringify(page.props) : ""
    }
    readonly property string paneKey: everWide ? keyOf(current) : ""
    // Later: taking a page away may save its draft, which changes what `current` reads.
    onPaneKeyChanged: Qt.callLater(pane.reload)
    // With a page above, the one below may still get its answer (a camera, a scan): it changes after.
    onIsCurrentPageChanged: {
        pane.reload()

        // The Back key keeps this page (ahead of the one shown), for how long nobody knows. With a
        // recording it comes back (onBackRequested), and the text goes with the recording.
        const place = host.Kirigami.ColumnView.index
        if (isCurrentPage || place < 0 || host.Kirigami.PageStack.pageStack.currentIndex >= place) {
            return
        }
        if (pane.item && pane.item.keepBegun && !pane.item.busy) {
            pane.item.keepBegun()
        }
    }
    // Sliding (disabled) the old page waits: let go now, its items turn enabled when its scope is
    // already gone (KanteScope.qml: TypeError in the log).
    onEnabledChanged: pane.reload()
    onWideChanged: {
        if (wide) {
            everWide = true
            return
        }
        // In use by itself beside the list (a recording, a field, a page it opened above): it stays
        // in sight.
        if (!chosen && pane.item && (pane.item.busy || typingInPane() || !isCurrentPage)) {
            chosen = current
        }
    }

    function typingInPane() {
        for (let item = host.Window.activeFocusItem; item; item = item.parent) {
            if (item === pane) {
                return true
            }
        }
        return false
    }

    // Back upright leads to the list, a recording beside it finishes first. PageRow has gone back
    // by then, whatever `accepted` says: the stack returns here (as ExamEndPage).
    onBackRequested: event => {
        const busy = pane.item && pane.item.busy
        if (!showsPage && !busy) {
            return
        }
        event.accepted = true
        const stack = host.Kirigami.PageStack.pageStack
        const place = host.Kirigami.ColumnView.index
        Qt.callLater(() => stack.currentIndex = place)
        if (!showsPage) {
            pane.item.finishBusy()
            return
        }
        // A field leaves the view: it saves as when it is left.
        if (typingInPane()) {
            host.Window.contentItem.forceActiveFocus()
        }
        choose(null)
    }

    // The window's head reads these, of a page on its stack.
    title: showsPage && pane.item ? pane.item.title : list.item ? list.item.title : ""
    actions: {
        if (showsPage && pane.item) {
            return Array.from(pane.item.actions)
        }
        const own = list.item ? Array.from(list.item.actions) : []
        return wide && pane.item ? own.concat(Array.from(pane.item.actions)) : own
    }
    readonly property var flickable: showsPage ? (pane.item ? pane.item.flickable ?? null : null) : list.item ? list.item.flickable : null
    // A new window (a change of style) comes back here (Main.keptPageList). `flickable` gives Main
    // one place; the page beside the list loads later and comes to its own.
    readonly property string keptPage: "ListAndPage.qml"
    readonly property var keptState: ({
        listSource: listSource,
        chosen: chosen,
        first: first,
        everWide: everWide,
        paneY: pane.item && pane.item.flickable ? pane.item.flickable.contentY : 0
    })
    property real paneY: 0
    // Later: on its way onto the stack this page has no window yet.
    function place(flickable, y) {
        if (flickable && y > 0) {
            Qt.callLater(() => host.Window.window.scrollTo(flickable, y))
        }
    }

    property Item lookScope: LookScope { target: host }
    background: Rectangle { color: Look.backgroundColor }
    padding: 0

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Loader {
            id: list

            objectName: "listPane"
            visible: !host.showsPage
            Layout.fillHeight: true
            Layout.fillWidth: !host.wide
            // Half of a small window: its page needs the room too.
            Layout.preferredWidth: host.wide ? Math.min(Look.unit(400), host.width / 2) : -1
            source: Qt.resolvedUrl(host.listSource)
            onLoaded: {
                item.paneHost = host
                host.takeFirst()
            }
        }
        Connections {
            target: list.item
            function onFirstPaneChanged() {
                host.takeFirst()
            }
        }

        Kirigami.Separator {
            objectName: "paneRule"
            visible: host.wide
            Layout.fillHeight: true
        }

        Item {
            visible: host.wide || host.showsPage
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: pane

                objectName: "pagePane"
                anchors.fill: parent

                property string loadedKey: ""
                function reload() {
                    // Off the stack (no view) Kirigami calls a page current: nothing new for one going.
                    if (!host.Kirigami.ColumnView.view || !host.isCurrentPage) {
                        return
                    }
                    if (loadedKey === host.paneKey || (item && !host.enabled)) {
                        return
                    }
                    loadedKey = host.paneKey
                    if (host.paneKey === "") {
                        source = ""
                        return
                    }
                    setSource(Qt.resolvedUrl(host.current.source), host.current.props)
                }
                onLoaded: {
                    if ("paneHost" in item) {
                        item.paneHost = host
                    }
                    host.place(item.flickable, host.paneY)
                    host.paneY = 0
                }
            }

            LookEmptyState {
                objectName: "paneEmpty"
                anchors.centerIn: parent
                width: parent.width - 2 * Look.unit(28)
                visible: !host.current
                title: qsTr("Choose a course from the list.")
            }
        }
    }

    Component.onCompleted: {
        everWide = everWide || wide
        pane.reload()
    }
}
