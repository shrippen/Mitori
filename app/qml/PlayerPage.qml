import QtQuick
import org.kde.kirigami as Kirigami
import Kante
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
    function go(direction) { view.go(direction) }
    function jump(ms) { view.jump(ms) }
    function step(direction) { view.step(direction) }
    function togglePlay() { view.togglePlay() }

    // The entry on screen, for the programme list to mark it when the user comes back.
    signal shown(string id)

    title: view.current.title || ""
    padding: 0
    // Kante's page ground (the window's scope gives Page the dialog colour).
    background: Rectangle { color: KanteStyle.backgroundColor }
    globalToolBarStyle: view.landscape ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.Auto

    onIndexChanged: view.index = index

    // Keys reach the view (Space, arrows, the transport bar's keys) as soon as the page opens.
    Keys.forwardTo: [view]

    PlayerView {
        id: view

        anchors.fill: parent
        focus: true
        playlist: page.playlist
        // Both ways: the view moves on (next, training), a caller may set the page's index.
        Component.onCompleted: index = page.index
        onIndexChanged: page.index = index
        onShown: id => page.shown(id)
        onBackRequested: page.Kirigami.PageStack.pop()
        onNotesRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
    }
}
