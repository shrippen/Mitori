import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
import Mitori

// The player as a page (PlayerView): from the programme, today's practice and the search; in
// landscape without the page header (the rail or the bands of PlayerView have its controls).
Kirigami.Page {
    id: page

    required property var playlist
    required property int index
    property alias training: view.training
    // Full screen: the window without bars, the screen sideways. Opened that way (from the small
    // player) the page goes when it ends.
    property bool big: false
    property bool bigOnly: false
    property alias startAt: view.startAt

    // For the callers and the UI test: the view's state and actions.
    readonly property alias current: view.current
    readonly property alias video: view.video
    readonly property alias source: view.source
    readonly property alias from: view.from
    readonly property alias to: view.to
    readonly property alias end: view.end
    readonly property alias frameMs: view.frameMs
    readonly property alias position: view.position
    readonly property alias playing: view.playing
    readonly property alias duration: view.duration
    readonly property alias rate: view.rate
    readonly property alias tempoText: view.tempoText
    property alias looping: view.looping
    function setSpeed(rate) { view.setSpeed(rate) }
    function seek(ms) { view.seek(ms) }
    readonly property alias trainingDone: view.trainingDone
    readonly property alias practisedCount: view.practisedCount
    readonly property alias hasVideo: view.hasVideo
    function go(direction) { view.go(direction) }
    function jump(ms) { view.jump(ms) }
    function step(direction) { view.step(direction) }
    function togglePlay() { view.togglePlay() }
    function showJump(right) { view.showJump(right) }

    // The entry on screen, for the programme list to mark it when the user comes back.
    signal shown(string id)
    // Full screen ended (bigOnly): where the small player goes on.
    signal bigEnded(real position, real rate, bool looping, bool playing)

    title: view.titleWithAddition
    padding: 0
    // The style's page ground (the window's scope gives Page the dialog colour).
    background: Rectangle { color: Look.backgroundColor }
    // Sideways the video takes the whole height: no head (Kirigami's bar, or Shinki's head).
    readonly property bool headless: view.landscape
    // Not Auto: a page takes the style the bar resolved it to; with Auto it had no head at all.
    globalToolBarStyle: headless ? Kirigami.ApplicationHeaderStyle.None
                                 : page.Kirigami.PageStack.pageStack?.globalToolBar.actualStyle ?? Kirigami.ApplicationHeaderStyle.None

    onIndexChanged: {
        view.index = index
        trainingList.positionViewAtIndex(index, ListView.Contain)    // the training moves on
    }

    // The window as it was (a maximised desktop window stays maximised afterwards).
    property var bigWindow: null
    property int windowBefore: Window.AutomaticVisibility
    function showBig() {
        if (big) {
            bigWindow = page.Window.window
            windowBefore = bigWindow.visibility
            bigWindow.visibility = Window.FullScreen
            Levels.setSideways(Levels.Switch.On)
            return
        }
        endBig()
    }
    function endBig() {
        Levels.setSideways(Levels.Switch.Off)
        bigWindow.visibility = windowBefore
    }
    onBigChanged: {
        showBig()
        // The button just hidden would keep the keys; a page left has none to give.
        if (isCurrentPage) {
            view.forceActiveFocus()
        }
    }
    // The Back key only moves the stack on and keeps this page: it ends full screen and the video.
    onIsCurrentPageChanged: {
        // Not while it is being made (index -1), nor under a page above it (the notes).
        const place = page.Kirigami.ColumnView.index
        if (isCurrentPage || place < 0 || page.Kirigami.PageStack.pageStack.currentIndex >= place) {
            return
        }
        if (bigOnly && big) {
            bigEnded(view.position, view.rate, view.looping, view.willPlay())
        }
        big = false
        view.pause()
    }
    // Popped while full screen.
    Component.onDestruction: {
        if (big) {
            endBig()
        }
    }

    // Back: a step to the page before, then this one goes at once (it stops playing). Kirigami's
    // pop() slides it out, and after the window turned (sideways) it stops that slide and keeps
    // the page, still playing.
    function leave() {
        const stack = page.Kirigami.PageStack.pageStack
        stack.currentIndex = page.Kirigami.ColumnView.index - 1
        stack.pop(stack.currentItem)
    }

    // Keys reach the view (Space, arrows, the transport bar's keys) as soon as the page opens.
    Keys.forwardTo: [view]

    // Sideways the training has its list beside the small player (clickdummy "session"): the
    // picture alone would leave no way to rate or go on.
    readonly property bool split: view.training && !big && width > height
    // The list opens at the current technique, not at the top.
    onSplitChanged: Qt.callLater(trainingList.positionViewAtIndex, view.index, ListView.Contain)

    RowLayout {
        anchors.fill: parent
        spacing: 0

    ListView {
        id: trainingList

        objectName: "trainingSideList"
        Layout.fillHeight: true
        Layout.preferredWidth: Math.min(page.width * 0.38, Kirigami.Units.gridUnit * 24)
        visible: page.split
        model: page.split ? page.playlist : []
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

        delegate: LookListRow {
            required property var modelData
            required property int index

            width: ListView.view.width
            leadingText: modelData.nr ? String(modelData.nr) : ""
            // ✓ what is done today, as the list under the player has it.
            text: (Practice.revision >= 0 && Practice.ratedToday(modelData.id) ? "✓ " : "")
                  + modelData.title + (modelData.extra && !modelData.nr ? " (" + modelData.extra + ")" : "")
            subtitle: [modelData.jp, modelData.nr ? modelData.extra : modelData.attack].filter(x => x).join(" · ")
            selected: index === view.index
            Accessible.name: text.replace("✓ ", "") + (Practice.revision >= 0 && Practice.ratedToday(modelData.id) ? ", " + qsTr("practised") : "")
            focusOnClick: false         // Space stays with the player
            onClicked: view.index = index
            trailing: LookLevelDots {
                value: Practice.revision >= 0 ? Practice.level(modelData.id) : 0
            }
        }
    }

    PlayerView {
        id: view

        Layout.fillWidth: true
        Layout.fillHeight: true
        compact: page.split
        focus: true
        playlist: page.playlist
        // Both ways: the view moves on (next, training), a caller may set the page's index. Bound
        // at first, so that the view starts at it (not at 0, loading another file first).
        index: page.index
        onIndexChanged: page.index = index
        onShown: id => page.shown(id)
        big: page.big
        onBackRequested: page.leave()
        onBigRequested: page.big = true
        onBigLeft: {
            if (page.bigOnly) {
                page.bigEnded(view.position, view.rate, view.looping, view.willPlay())
                page.leave()
                return
            }
            page.big = false
        }
        // It rests under the notes: no sound over the reading, and none into a voice note.
        onNotesRequested: entry => {
            view.pause()
            page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
        }
    }
    }
}
