pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// Programme (clickdummy "Programm"): discipline, grade, and the grade's own material. A row opens
// the technique sheet first; ▶ goes straight to the player. On a tablet in landscape
// (app-entscheidungen.md, "Tablet quer") list and small player are one page: a row with a video
// plays beside the list, one without opens the sheet. Opened, the page is silent: the video stands
// until the first tap. A grade without forms (Hon-Menkyo) shows a card instead of the list; it
// stays on this page, like every grade browsed here.
Kirigami.Page {
    id: page

    // The page keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    readonly property string discipline: Programme.discipline
    readonly property var grades: Programme.gradeChips(discipline)
    // The grade being browsed: starts at the practice grade, then only this page's. Changes nothing
    // on the start menu, today's practice or the exam mode (PracticeGrade).
    property string grade: PracticeGrade.grade(discipline)
    readonly property var chosenGrade: grades.find(g => g.id === grade) || ({})
    readonly property bool noForms: chosenGrade.count === 0
    readonly property var rows: Programme.entries(discipline, grade)
    // Whether the grade has video positions at all, and whether any of their videos is on the device.
    readonly property bool hasPositions: rows.some(r => !!r.video)
    readonly property bool hasVideo: rows.some(r => page.playable(r))
    // The entry shown last in the player, marked when the list comes back (on a tablet: the one
    // playing beside the list).
    property string lastId: ""
    // On a phone that row lights up for a moment on the way back, and comes into view.
    property string returnedId: ""
    property bool cameFromPlayer: false       // only from the player: from the notes the list stays put
    // A moment later: at once the list, shown again, goes back to its top.
    Timer {
        id: returnedInView

        interval: 50
        onTriggered: list.positionViewAtIndex(page.rows.findIndex(r => r.id === page.returnedId), ListView.Contain)
    }
    Timer {
        id: returnedOff

        interval: 1600
        onTriggered: page.returnedId = ""
    }
    // A tablet in landscape: a page at least 26 grid units high (a 600 dp tablet less its bars;
    // a phone held sideways has about 20) keeps the list and the small player side by side.
    readonly property bool wide: width > height && height >= 460     // a tablet sideways, not a phone
    readonly property var playlist: rows.filter(r => page.playable(r))
    readonly property bool split: wide && playlist.length > 0
    // The small player beside the list (tablet), typed for the calls below.
    readonly property PlayerView player: pane.item as PlayerView
    // The first tap came (play control, a row, full screen): chosen techniques now play at once.
    // Here, not in the player: turning the tablet makes the pane anew.
    property bool started: false
    // What the small player left when it went, for the pane made anew (same reason):
    // { id, rate, looping, position, playing, source }; taken once.
    property var paneRest: null
    // Follows split, one step behind: the pane still says where it stood before it goes (a Loader
    // has ended its item's functions by the time anyone hears of it).
    property bool paneOn: false
    onSplitChanged: {
        if (!split && player) {
            paneRest = player.restState()
        }
        paneOn = split
    }
    Component.onCompleted: paneOn = split

    title: qsTr("Programme")

    actions: [
        Kirigami.Action {
            icon.name: "search"
            text: qsTr("Search")
            onTriggered: page.Kirigami.PageStack.push(Qt.resolvedUrl("SearchPage.qml"))
        },
        Kirigami.Action {
            icon.name: "view-list-details"
            text: qsTr("All notes")
            onTriggered: page.Kirigami.PageStack.push(Qt.resolvedUrl("AllNotesPage.qml"))
        }
    ]

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        LookSegmented {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            model: Programme.disciplines.map(d => d.kurzname)
            currentIndex: Programme.disciplines.findIndex(d => d.id === page.discipline)
            onActivated: index => {
                Programme.discipline = Programme.disciplines[index].id
                page.grade = PracticeGrade.grade(Programme.discipline)
            }
        }

        Flow {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.smallSpacing
            Layout.rightMargin: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                objectName: "gradeChips"
                model: page.grades

                LookChip {
                    required property var modelData

                    // How far the grade is (clickdummy `.gchip`): exam-ready of all its techniques.
                    readonly property var ids: Programme.entries(page.discipline, modelData.id)
                        .filter(r => r.kind === "entry").map(r => r.id)
                    readonly property int ready: Practice.revision >= 0 ? Practice.counts(ids)[4] : 0
                    text: ids.length ? modelData.name + " " + ready + "/" + ids.length : modelData.name + " –"
                    Accessible.name: ids.length ? qsTr("%1, %2 of %3 exam-ready").arg(modelData.name).arg(ready).arg(ids.length)
                                                : qsTr("%1, no forms yet").arg(modelData.name)
                    checkable: true
                    choice: true        // the page holds the grade: a tap only reports it

                    checked: modelData.id === page.grade
                    onClicked: page.grade = modelData.id
                }
            }
        }
    }

    padding: 0

    RowLayout {
        anchors.fill: parent
        spacing: 0

        LookScrollView {
            objectName: "programmeScroll"
            clip: true                                  // the rows scroll under the grade chips, not over them
            Layout.fillHeight: true
            Layout.fillWidth: !page.split
            Layout.preferredWidth: page.split ? Kirigami.Units.gridUnit * 20 : -1
            ListView {
                id: list

                objectName: "programmeList"
                // The head above the rows changes height (its width, another grade): a list standing
                // at its top stays there instead of cutting off the first line. One scrolled
                // anywhere else, by hand or to the playing row, is left alone; also when that
                // happens before the moment is over (back from the player).
                property real topBefore: 0
                onOriginYChanged: {
                    if (!moving && contentY === topBefore) {
                        const stood = contentY
                        Qt.callLater(() => {
                            // No list: the page has gone by now.
                            if (list && list.contentY === stood) {
                                list.positionViewAtBeginning()
                            }
                        })
                    }
                    topBefore = originY
                }
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

                    // A grade's meaning, content and waiting time (Shinki Toho has them).
                    LookLabel {
                        objectName: "gradeInfo"
                        Layout.fillWidth: true
                        Layout.margins: Kirigami.Units.smallSpacing
                        readonly property var grade: page.chosenGrade
                        visible: !!grade.bedeutung && !page.noForms
                        text: [grade.bedeutung, grade.inhalt, grade.abstand ? qsTr("Waiting time: %1").arg(grade.abstand) : ""]
                              .filter(x => x).join(" · ")
                        color: Look.mutedTextColor
                        wrapMode: Text.Wrap
                    }
                    LookLabel {
                        objectName: "kanjiProposal"
                        Layout.fillWidth: true
                        Layout.margins: Kirigami.Units.smallSpacing
                        visible: !!(Programme.disciplines.find(d => d.id === page.discipline) || {}).kanji_vorschlag
                        text: qsTr("The Japanese writing of these names is a proposal, not yet confirmed.")
                        color: Look.mutedTextColor
                        font: Look.smallFont
                        wrapMode: Text.Wrap
                    }
                    // A grade without forms: what the catalog knows of it. On a tablet no player
                    // stands beside it (nothing to play), and the card stays at a width that reads.
                    QQC2.Control {
                        objectName: "gradeCard"
                        Layout.fillWidth: true
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
                        Layout.margins: Kirigami.Units.smallSpacing
                        visible: page.noForms
                        padding: Kirigami.Units.largeSpacing
                        background: LookCard {}

                        contentItem: ColumnLayout {
                            spacing: Kirigami.Units.largeSpacing

                            LookGroupHead {
                                Layout.fillWidth: true
                                title: page.chosenGrade.name || ""
                                japanese: page.chosenGrade.jp || ""
                            }
                            Repeater {
                                model: [{ label: qsTr("Meaning"), value: page.chosenGrade.bedeutung },
                                        { label: qsTr("Content"), value: page.chosenGrade.inhalt },
                                        { label: qsTr("Waiting time"), value: page.chosenGrade.abstand }].filter(f => f.value)

                                LookFormRow {
                                    id: fact

                                    required property var modelData

                                    label: modelData.label

                                    LookLabel {
                                        Layout.fillWidth: true
                                        text: fact.modelData.value
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                            LookLabel {
                                objectName: "gradeCardNote"
                                Layout.fillWidth: true
                                text: qsTr("The forms of this grade are not in the app yet.")
                                color: Look.mutedTextColor
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                    GradeStand {
                        objectName: "gradeStand"
                        Layout.fillWidth: true
                        Layout.margins: Kirigami.Units.smallSpacing
                        entries: page.rows
                    }

                    LookHint {
                        Layout.fillWidth: true
                        Layout.margins: Kirigami.Units.smallSpacing
                        visible: !page.hasVideo && !page.noForms
                        importance: LookHint.Importance.Info
                        titleText: qsTr("Exam videos")
                        text: !page.hasPositions
                              ? qsTr("There is no video for this grade.")
                              : qsTr("The videos are on the DVDs of the Shinkiryu association. With the DVD's code Mitori loads them onto this device.")

                        // One below the other on a small phone: side by side they would widen the list.
                        GridLayout {
                            Layout.fillWidth: true
                            columns: page.width < Kirigami.Units.gridUnit * 24 ? 1 : 2

                            LookButton {
                                objectName: "enterCode"
                                visible: page.hasPositions
                                text: qsTr("Enter code")
                                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
                            }
                            LookButton {
                                visible: page.hasPositions
                                text: qsTr("Order on shinkiryu.de")
                                onClicked: Qt.openUrlExternally("https://www.shinkiryu.de/shinkiryu/de/DVD.php")
                            }
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

                            LookGroupHead {
                                id: heading

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: Kirigami.Units.largeSpacing
                                anchors.rightMargin: Kirigami.Units.largeSpacing
                                anchors.bottomMargin: Kirigami.Units.smallSpacing
                                title: rowLoader.modelData.title
                                japanese: rowLoader.modelData.jp || ""
                            }
                        }
                    }

                    Component {
                        id: attackRow

                        LookLabel {
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

                        LookListRow {
                            id: entryItem

                            readonly property var entry: rowLoader.modelData
                            readonly property bool playable: page.playable(entry)

                            leadingText: entry.nr ? String(entry.nr) : ""
                            text: entry.title + (entry.kind === "entry" && entry.extra && !entry.nr ? " (" + entry.extra + ")" : "")
                            subtitle: [entry.jp, entry.nr ? entry.extra : entry.text].filter(x => x).join(" · ")
                            selected: entry.id === (page.split ? page.lastId : page.returnedId)
                            onClicked: page.open(entry)
                            Keys.onTabPressed: event => page.tabRow(rowLoader.index, 1, event)
                            Keys.onBacktabPressed: event => page.tabRow(rowLoader.index, -1, event)
                            trailing: RowLayout {
                                spacing: Kirigami.Units.smallSpacing

                                LookLevelDots {
                                    visible: entryItem.entry.kind === "entry"
                                    value: Practice.revision >= 0 ? Practice.level(entryItem.entry.id) : 0
                                    names: Practice.levelNames
                                    color: Look.levelColor(value)     // as in the grade's bar and legend
                                }
                                LookPlayButton {
                                    // Without a video the place stays empty: the dots stay in line.
                                    objectName: "rowPlay"
                                    opacity: entryItem.playable ? 1 : 0
                                    enabled: entryItem.playable
                                    activeFocusOnTab: false     // Enter on the row: the sheet plays it
                                    text: qsTr("Play")
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
            active: page.paneOn
            visible: active
            Layout.fillWidth: true
            Layout.fillHeight: true
            onLoaded: {
                page.restorePane()
                page.resumeFromBig()    // second: what full screen left is newer
            }

            sourceComponent: PlayerView {
                id: paneView

                compact: true
                autoPlay: page.isCurrentPage && page.started
                onPlayTapped: page.started = true
                playlist: page.playlist
                // The chosen technique (also after the pane is made anew, turning the tablet).
                index: Math.max(0, page.playlist.findIndex(r => r.id === page.lastId))
                // First values only: setList changes both in one step, bindings one after the other.
                Component.onCompleted: {
                    paneView.playlist = page.playlist
                    paneView.index = paneView.index
                }
                onShown: id => {
                    if (id) {
                        page.lastId = id
                    }
                }
                onBigRequested: page.openBig()
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
    // Nothing to play (the grade card): the pane goes as it stands and comes back so.
    onPlaylistChanged: {
        if (page.player && page.playlist.length > 0) {
            page.player.setList(page.playlist, page.lastId)
        }
    }
    // Under another page (notes, comment) the small player rests.
    onIsCurrentPageChanged: {
        if (!isCurrentPage && page.player) {
            page.player.pause()
        }
        if (isCurrentPage && page.cameFromPlayer && !page.split && page.lastId !== "") {
            const row = page.rows.findIndex(r => r.id === page.lastId)
            if (row >= 0) {
                page.returnedId = page.lastId
                returnedOff.restart()
                returnedInView.start()
            }
        }
        if (isCurrentPage) {
            page.cameFromPlayer = false
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
        bigResume = null        // another technique: what full screen left is no longer its place
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
            page.playBeside(entry)
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
        bigResume = null
        if (page.split) {
            page.playBeside(entry)
            return
        }
        page.paneRest = { playing: false, looping: true }   // a new player page starts anew, and so does the pane after it; it stands
        page.cameFromPlayer = true
        const playlist = page.rows.filter(r => page.playable(r))
        // The window's page stack returns the new page (the attached PageStack.push returns nothing).
        const window = page.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow
        const player = window.pageStack.push(Qt.resolvedUrl("PlayerPage.qml"), {
            playlist: playlist, index: playlist.findIndex(r => r.id === entry.id)
        }) as PlayerPage
        page.lastId = entry.id
        player.shown.connect(id => page.lastId = id)
    }

    // A row beside the list. It also starts the technique standing there: its row is already
    // chosen, so nothing else would make it play.
    function playBeside(entry) {
        const standing = page.player && page.player.current.id === entry.id && !page.player.playing
        page.started = true
        page.lastId = entry.id
        if (standing) {
            page.player.togglePlay()
        }
    }

    // A pane made anew (the tablet turned, the window narrowed, back from the grade card): playing
    // plays on, standing stands, the loop stays. The same technique keeps its tempo and place; another
    // one chosen meanwhile starts at 1×. A new file of it waits at the chapter's start, as in an open
    // player, also when full screen left a place in the old one.
    function restorePane() {
        const now = page.player
        const rest = page.paneRest
        page.paneRest = null
        if (!now || !rest) {
            return
        }

        now.holdStart = !rest.playing
        now.looping = rest.looping
        if (rest.id !== now.current.id) {
            return
        }

        now.setSpeed(rest.rate)
        if (rest.source === now.source.toString()) {
            now.startAt = rest.position
        } else {
            now.holdStart = true
            page.bigResume = null
        }
    }

    // The small player goes full screen where it stands; back here it stays where it was, playing
    // or paused as full screen left it. The pane may be gone by then (a window turned for full screen is no longer wide): the values
    // wait until it is made anew.
    property var bigResume: null
    function resumeFromBig() {
        const now = page.player
        if (!now || !bigResume) {
            return
        }
        now.holdStart = !bigResume.playing
        now.looping = bigResume.looping
        now.setSpeed(bigResume.rate)
        if (now.duration > 0) {
            now.seek(bigResume.position)
            if (bigResume.playing) {
                now.resume()
            }
        } else {
            now.startAt = bigResume.position    // still loading: restart() would go to the start
        }
        bigResume = null
    }
    function openBig() {
        page.started = true     // full screen plays
        page.cameFromPlayer = true
        const small = page.player
        const window = page.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow
        const player = window.pageStack.push(Qt.resolvedUrl("PlayerPage.qml"), {
            playlist: small.playlist, index: small.index, bigOnly: true, startAt: small.position, looping: small.looping
        }) as PlayerPage
        player.setSpeed(small.rate)
        player.big = true
        // Back in the small player: the same place, tempo and loop.
        player.bigEnded.connect((position, rate, looping, playing) => {
            bigResume = { position: position, rate: rate, looping: looping, playing: playing }
            resumeFromBig()
        })
        player.shown.connect(id => page.lastId = id)
    }

    // Reads Videos.folder so that bindings calling it follow a new folder from the settings.
    function playable(row) {
        return Videos.folder !== undefined && row.kind === "entry" && !!row.video && Videos.available(row.video.video)
    }
}
