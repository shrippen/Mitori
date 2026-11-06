import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// A list page and, on a tablet sideways, the page chosen in it beside it (the clickdummy's `.split2`).
// Elsewhere only the list; its rows open their pages on the window's stack.
//
//   ┌ list ─────┬ page ─────────────┐
//   │ rows …    │ the row's page    │
//   └───────────┴───────────────────┘
//
// A list page and a page beside it may have `paneHost`: then they get this page. A list asks
// show(file, props) and pushes the page itself when it answers false; it may name `firstPane`,
// what stands beside it before a row is chosen. A pushed page finds the window's stack as usual.
Kirigami.Page {
    id: host

    required property string listSource
    // The row chosen: { source, props }, null until then.
    property var chosen: null
    readonly property var current: chosen || (list.item ? list.item.firstPane ?? null : null)

    readonly property TypingHold hold: TypingHold { target: host }
    readonly property bool wide: hold.wide

    // Whether a page is the one beside the list; with `uid` that notebook of it.
    function isShown(source, uid) {
        return wide && !!current && current.source === source && (uid === undefined || current.props.uid === uid)
    }
    function show(source, props) {
        if (!wide) {
            return false
        }
        chosen = { source: source, props: props || {} }
        return true
    }
    // The page beside the list went by itself (its notebook was deleted).
    function clearPane() {
        chosen = null
    }

    readonly property string paneKey: wide && current ? current.source + JSON.stringify(current.props) : ""
    // Later: taking a page away may save its draft, which changes what `current` reads.
    onPaneKeyChanged: Qt.callLater(pane.reload)
    onWideChanged: {
        // Turned upright: the chosen page goes on as a page of its own.
        if (!wide && chosen && isCurrentPage) {
            const page = chosen
            chosen = null
            host.Kirigami.PageStack.push(Qt.resolvedUrl(page.source), page.props)
        }
    }

    // The window's head reads these, of a page on its stack.
    title: list.item ? list.item.title : ""
    actions: {
        const own = list.item ? Array.from(list.item.actions) : []
        return wide && pane.item ? own.concat(Array.from(pane.item.actions)) : own
    }
    readonly property var flickable: list.item ? list.item.flickable : null
    // A new window (a change of style) comes back here (Main.keptPageList).
    readonly property string keptPage: "ListAndPage.qml"
    readonly property var keptState: ({ listSource: listSource, chosen: chosen })

    property Item lookScope: LookScope { target: host }
    background: Rectangle { color: Look.backgroundColor }
    padding: 0

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Loader {
            id: list

            objectName: "listPane"
            Layout.fillHeight: true
            Layout.fillWidth: !host.wide
            // Half of a small window: its page needs the room too.
            Layout.preferredWidth: host.wide ? Math.min(Look.unit(400), host.width / 2) : -1
            source: Qt.resolvedUrl(host.listSource)
            onLoaded: item.paneHost = host
        }

        Kirigami.Separator {
            objectName: "paneRule"
            visible: host.wide
            Layout.fillHeight: true
        }

        Item {
            visible: host.wide
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: pane

                objectName: "pagePane"
                anchors.fill: parent
                visible: host.wide

                function reload() {
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

    Component.onCompleted: pane.reload()
}
