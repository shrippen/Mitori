import QtQuick
import org.kde.kirigami as Kirigami
import Mitori

// The player as a page (PlayerView): from the programme, today's practice and the search; in
// landscape without the page header (the controls stand in a column on the right).
Kirigami.Page {
    id: page

    required property var playlist
    required property int index
    property alias training: view.training

    // For the callers and the UI test: the view's state and actions.
    readonly property alias current: view.current
    readonly property alias video: view.video
    readonly property alias from: view.from
    readonly property alias to: view.to
    readonly property alias end: view.end
    readonly property alias frameMs: view.frameMs
    readonly property alias position: view.position
    readonly property alias playing: view.playing
    readonly property alias duration: view.duration
    readonly property alias rate: view.rate
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

    title: view.current.title || ""
    padding: 0
    // The style's page ground (the window's scope gives Page the dialog colour).
    background: Rectangle { color: Look.backgroundColor }
    // Sideways the video takes the whole height: no head (Kirigami's bar, or Shinki's head).
    readonly property bool headless: view.landscape
    globalToolBarStyle: headless ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.Auto

    onIndexChanged: view.index = index

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

    PlayerView {
        id: view

        anchors.fill: parent
        focus: true
        playlist: page.playlist
        // Both ways: the view moves on (next, training), a caller may set the page's index. Bound
        // at first, so that the view starts at it (not at 0, loading another file first).
        index: page.index
        onIndexChanged: page.index = index
        onShown: id => page.shown(id)
        onBackRequested: page.leave()
        onNotesRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
    }
}
