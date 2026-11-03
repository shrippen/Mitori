pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Programme (clickdummy "Programm"): discipline, grade, and the grade's own material. A row opens
// the technique sheet first; ▶ goes straight to the player. On a tablet in landscape
// (app-entscheidungen.md, "Tablet quer") list and small player are one page: a row with a video
// plays beside the list, one without opens the sheet.
Kirigami.Page {
    id: page

    // The page keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    readonly property string discipline: Programme.discipline
    readonly property var grades: Programme.grades(discipline)
    property string grade: Programme.grade(discipline)
    readonly property var rows: Programme.entries(discipline, grade)
    // Whether the grade has video positions at all, and whether any of their videos is on the device.
    readonly property bool hasPositions: rows.some(r => !!r.video)
    readonly property bool hasVideo: rows.some(r => page.playable(r))
    // The entry shown last in the player, marked when the list comes back (on a tablet: the one
    // playing beside the list).
    property string lastId: ""
    // A tablet in landscape: a page at least 26 grid units high (a 600 dp tablet less its bars;
    // a phone held sideways has about 20) keeps the list and the small player side by side.
    readonly property bool wide: width > height && height >= Kirigami.Units.gridUnit * 26
    readonly property var playlist: rows.filter(r => page.playable(r))
    readonly property bool split: wide && playlist.length > 0
    // The small player beside the list (tablet), typed for the calls below.
    readonly property PlayerView player: pane.item as PlayerView

    title: qsTr("Programme")

    actions: [
        Kirigami.Action {
            icon.name: "view-list-details"
            text: qsTr("All notes")
            onTriggered: page.Kirigami.PageStack.push(Qt.resolvedUrl("AllNotesPage.qml"))
        }
    ]

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        KanteSegmented {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            model: Programme.disciplines.map(d => d.kurzname)
            currentIndex: Programme.disciplines.findIndex(d => d.id === page.discipline)
            onActivated: index => {
                Programme.discipline = Programme.disciplines[index].id
                page.grade = Programme.grade(Programme.discipline)
            }
        }

        Flow {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.smallSpacing
            Layout.rightMargin: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: page.grades

                KanteChip {
                    required property var modelData

                    text: modelData.name
                    checkable: true
                    checked: modelData.id === page.grade
                    onClicked: {
                        Programme.setGrade(page.discipline, modelData.id)
                        page.grade = modelData.id
                    }
                }
            }
        }
    }

    padding: 0

    RowLayout {
        anchors.fill: parent
        spacing: 0

        QQC2.ScrollView {
            objectName: "programmeScroll"
            clip: true                                  // the rows scroll under the grade chips, not over them
            Layout.fillHeight: true
            Layout.fillWidth: !page.split
            Layout.preferredWidth: page.split ? Kirigami.Units.gridUnit * 20 : -1
            ListView {
                id: list

                objectName: "programmeList"
                model: page.rows
                reuseItems: true
                // Up and Down scroll (the page's keys) also from a focused row; the list has no
                // current item of its own to move.
                keyNavigationEnabled: false
                Keys.onPressed: event => page.scrollKey(event)

                // Scrolls with the list, so a small screen keeps room for the techniques.
                header: ColumnLayout {
                    width: ListView.view.width
                    spacing: Kirigami.Units.smallSpacing

                    GradeStand {
                        objectName: "gradeStand"
                        Layout.fillWidth: true
                        Layout.margins: Kirigami.Units.smallSpacing
                        entries: page.rows
                    }

                    KanteHintCard {
                        Layout.fillWidth: true
                        Layout.margins: Kirigami.Units.smallSpacing
                        visible: !page.hasVideo
                        tier: KanteHintCard.Tier.Blue
                        titleText: qsTr("Exam videos")
                        text: !page.hasPositions
                              ? qsTr("There is no video for this grade.")
                              : qsTr("The videos are on the DVDs of the Shinkiryu association. With the DVD's code Mitori loads them onto this device.")

                        KanteButton {
                            objectName: "enterCode"
                            visible: page.hasPositions
                            text: qsTr("Enter code")
                            onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
                        }
                        KanteButton {
                            visible: page.hasPositions
                            text: qsTr("Order on shinkiryu.de")
                            onClicked: Qt.openUrlExternally("https://www.shinkiryu.de/shinkiryu/de/DVD.php")
                        }
                    }
                }

                delegate: Loader {
                    id: rowLoader

                    required property var modelData
                    required property int index

                    width: ListView.view.width
                    sourceComponent: modelData.kind === "group" ? groupRow
                                   : modelData.kind === "attack" || modelData.kind === "text" ? attackRow : entryRow

                    Component {
                        id: groupRow

                        Item {
                            implicitHeight: heading.implicitHeight + Kirigami.Units.largeSpacing * 2

                            KanteHeading {
                                id: heading

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: Kirigami.Units.largeSpacing
                                anchors.rightMargin: Kirigami.Units.largeSpacing
                                anchors.bottomMargin: Kirigami.Units.smallSpacing
                                level: 4
                                text: rowLoader.modelData.title + "  " + rowLoader.modelData.jp
                            }
                        }
                    }

                    Component {
                        id: attackRow

                        QQC2.Label {
                            text: rowLoader.modelData.title + (rowLoader.modelData.extra ? " (" + rowLoader.modelData.extra + ")" : "")
                                  + "  " + (rowLoader.modelData.jp || "")
                            font.bold: rowLoader.modelData.kind === "attack"
                            topPadding: Kirigami.Units.smallSpacing
                            leftPadding: Kirigami.Units.largeSpacing
                            wrapMode: Text.Wrap
                        }
                    }

                    Component {
                        id: entryRow

                        KanteListRow {
                            id: entryItem

                            readonly property var entry: rowLoader.modelData
                            readonly property bool playable: page.playable(entry)

                            leadingText: entry.nr ? String(entry.nr) : ""
                            text: entry.title + (entry.kind === "entry" && entry.extra && !entry.nr ? " (" + entry.extra + ")" : "")
                            subtitle: [entry.jp, entry.nr ? entry.extra : (entry.kind === "info" ? entry.text : "")].filter(x => x).join(" · ")
                            selected: entry.id === page.lastId
                            onClicked: page.open(entry)
                            Keys.onTabPressed: event => page.tabRow(rowLoader.index, 1, event)
                            Keys.onBacktabPressed: event => page.tabRow(rowLoader.index, -1, event)
                            trailing: RowLayout {
                                spacing: Kirigami.Units.smallSpacing

                                KanteLevelDots {
                                    visible: entryItem.entry.kind === "entry"
                                    value: Practice.revision >= 0 ? Practice.level(entryItem.entry.id) : 0
                                    names: Practice.levelNames
                                    color: KanteStyle.levelColor(value)     // as in the grade's bar and legend
                                }
                                KanteToolButton {
                                    visible: entryItem.playable
                                    activeFocusOnTab: false     // Enter on the row: the sheet plays it
                                    icon.name: "media-playback-start"
                                    text: qsTr("Play")
                                    display: QQC2.AbstractButton.IconOnly
                                    onClicked: page.play(entryItem.entry)
                                }
                            }
                        }
                    }
                }
            }
        }

        Kirigami.Separator {
            visible: page.split
            Layout.fillHeight: true
        }

        // The small player beside the list (tablet).
        Loader {
            id: pane

            objectName: "playerPane"
            active: page.split
            visible: active
            Layout.fillWidth: true
            Layout.fillHeight: true

            sourceComponent: PlayerView {
                compact: true
                autoPlay: page.isCurrentPage
                playlist: page.playlist
                // The chosen technique (also after the pane is made anew, turning the tablet); bound
                // only until the player moves on by itself (next, previous).
                index: Math.max(0, page.playlist.findIndex(r => r.id === page.lastId))
                onShown: id => {
                    if (id) {
                        page.lastId = id
                    }
                }
                onCommentRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("CommentPage.qml"), { entry: entry })
                onNotesRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
            }
        }
    }

    TechniqueSheet {
        id: sheet

        objectName: "techniqueSheet"
        onPlayRequested: entry => page.play(entry)
        onCommentRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("CommentPage.qml"), { entry: entry })
        onNotesRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
        onVideosRequested: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
    }

    // Another grade or discipline: the technique shown last if it is in it, else its first.
    onPlaylistChanged: {
        if (page.player) {
            page.player.index = Math.max(0, page.playlist.findIndex(r => r.id === page.lastId))
        }
    }
    // Under another page (notes, comment) the small player rests.
    onIsCurrentPageChanged: {
        if (!isCurrentPage && page.player) {
            page.player.pause()
        }
    }
    // The keys scroll the list as ScrollablePage did (Up, Down, Page Up, Page Down, Home, End).
    // Keys the small player leaves (Up, Down, Page Up, Page Down, End) stay with it.
    function scrollBy(dy) {
        list.contentY = Math.max(list.originY, Math.min(list.originY + list.contentHeight - list.height, list.contentY + dy))
    }
    function inPlayer() {
        return page.inside(page.Window.activeFocusItem, pane)
    }
    // From the page and from a focused row (the list passes them on before its ScrollView, which
    // would scroll a whole screen).
    function scrollKey(event) {
        if (inPlayer()) {
            return
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
            scrollBy((event.key === Qt.Key_Down ? 1 : -1) * Kirigami.Units.gridUnit * 2)
        } else if (event.key === Qt.Key_PageUp || event.key === Qt.Key_PageDown) {
            scrollBy((event.key === Qt.Key_PageDown ? 1 : -1) * list.height * 0.9)
        } else if (event.key === Qt.Key_Home) {
            list.positionViewAtBeginning()
        } else if (event.key === Qt.Key_End) {
            list.positionViewAtEnd()
            endAgain.start()
        } else {
            return
        }
        event.accepted = true
    }
    Keys.onPressed: event => scrollKey(event)
    // The rows differ in height: until the list made the last ones, its height is an estimate and
    // End stopped a few pixels short. Once they are made, once more.
    Timer {
        id: endAgain
        interval: 0
        onTriggered: {
            if (!list.atYEnd) {
                list.positionViewAtEnd()
            }
        }
    }

    // A row chosen in the list plays beside it; the list shows the row of the technique playing.
    onLastIdChanged: {
        const i = page.playlist.findIndex(r => r.id === page.lastId)
        if (page.player && i >= 0 && page.player.index !== i) {
            page.player.index = i
        }
        const row = page.rows.findIndex(r => r.id === page.lastId)
        if (page.split && row >= 0) {
            list.positionViewAtIndex(row, ListView.Contain)
        }
    }

    // Tab and Shift+Tab from row to row in the list's order. The list reuses its rows, so the
    // window's focus chain (the order of the rows as items) left the list after a row or two.
    // Past the last row, or before the first, the focus leaves the list.
    function tabRow(from, step, event) {
        const reason = step > 0 ? Qt.TabFocusReason : Qt.BacktabFocusReason
        event.accepted = true
        for (let i = from + step; i >= 0 && i < page.rows.length; i += step) {
            const kind = page.rows[i].kind
            if (kind !== "entry" && kind !== "info") {
                continue
            }
            list.positionViewAtIndex(i, ListView.Contain)
            const row = list.itemAtIndex(i) as Loader
            if (row && row.item) {
                (row.item as Item).forceActiveFocus(reason)
            }
            return
        }
        // The next item of the window's chain outside the list (the small player, the header).
        let item = page.Window.activeFocusItem
        for (let n = 0; item && n < 1000; n++) {
            item = item.nextItemInFocusChain(step > 0)
            if (item && !page.inside(item, list)) {
                item.forceActiveFocus(reason)
                return
            }
        }
    }
    function inside(item, ancestor) {
        for (let p = item; p; p = p.parent) {
            if (p === ancestor) {
                return true
            }
        }
        return false
    }

    function open(entry) {
        if (page.split && page.playable(entry)) {
            page.lastId = entry.id
            return
        }
        if (entry.kind === "info") {
            page.Kirigami.PageStack.push(Qt.resolvedUrl("CommentPage.qml"), { entry: entry })
            return
        }
        sheet.entry = entry
        sheet.playable = page.playable(entry)
        sheet.open()
    }

    function play(entry) {
        sheet.close()
        if (page.split) {
            page.lastId = entry.id
            return
        }
        const playlist = page.rows.filter(r => page.playable(r))
        // The window's page stack returns the new page (the attached PageStack.push returns nothing).
        const window = page.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow
        const player = window.pageStack.push(Qt.resolvedUrl("PlayerPage.qml"), {
            playlist: playlist, index: playlist.findIndex(r => r.id === entry.id)
        }) as PlayerPage
        page.lastId = entry.id
        player.shown.connect(id => page.lastId = id)
    }

    // Reads Videos.folder so that bindings calling it follow a new folder from the settings.
    function playable(row) {
        return Videos.folder !== undefined && row.kind === "entry" && !!row.video && Videos.available(row.video.video)
    }
}
