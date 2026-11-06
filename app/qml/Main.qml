import QtQuick
import org.kde.kirigami as Kirigami
import Kante
import Mitori

Kirigami.ApplicationWindow {
    id: root

    title: "Mitori"
    width: KanteStyle.unit(420)
    height: KanteStyle.unit(800)

    // The style the window is built in (Appearance): Breeze and Material are the platform's
    // colours, Kante dark, Leinen light, Shinki the club newspaper's (paper, ink, one red).
    // Shinki and Kante are two themes of their own: neither takes the other's colours.
    readonly property bool platformStyle: Appearance.running === Appearance.Style.Breeze
                                          || Appearance.running === Appearance.Style.Material
    readonly property bool shinkiStyle: Appearance.running === Appearance.Style.Shinki
    Binding {
        target: KanteStyle
        property: "kind"
        // Under Shinki the Kante components (the few that are left) follow the window's theme,
        // which carries Shinki's roles; Kante's own palette is only for Kante and Leinen.
        value: root.platformStyle || root.shinkiStyle ? KanteStyle.Kind.System : KanteStyle.Kind.Kante
    }
    Binding {
        target: KanteStyle
        property: "preferDark"
        value: Appearance.running === Appearance.Style.Kante
    }
    Binding {
        target: KanteStyle
        property: "preferLight"
        value: Appearance.running === Appearance.Style.Leinen
    }
    // The scope gives the window's controls the colours of the style: Kante's under Kante and
    // Leinen, Shinki's under Shinki (LookScope). Never both.
    LookScope { target: root.contentItem }

    // One page at a time, also in landscape: the tablet gets its own page with list and player
    // (app-entscheidungen.md, "Tablet quer"), not Kirigami's columns.
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    // Under Shinki the window carries Shinki's head in place of Kirigami's bar (bible section 5,
    // Kopf); the other styles keep the bar (LookHead is empty there).
    pageStack.globalToolBar.style: root.shinkiStyle ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.Auto
    header: LookHead {
        objectName: "lookHead"
        page: root.pageStack.currentItem as Kirigami.Page
        // The page before the current one (back by key keeps the pages ahead).
        backTitle: root.pageStack.currentIndex > 0 ? root.pageStack.get(root.pageStack.currentIndex - 1).title || "" : ""
        moreText: qsTr("More")
        logo: root.pageLogo(root.pageStack.currentItem)
        onBack: root.back()
    }

    // The mark of the app: the page names it (the start menu has one), the head shows it.
    function pageLogo(page) {
        // The index, not the property: the page is a C++ type, a direct access to a property it
        // does not have would fail.
        const logo = page ? page["headLogo"] : ""
        return logo !== undefined ? logo : ""
    }

    // Back from Shinki's head: as the Back key (goBack, the page may ask first), then the page
    // left is dropped (a player stops at once). A page that asks takes the stack back a moment
    // later (ExamEndPage, Qt.callLater before this one); then it stays.
    function back() {
        const leaving = root.pageStack.currentIndex
        root.pageStack.goBack()
        Qt.callLater(() => {
            if (root.pageStack.currentIndex < leaving) {
                root.pageStack.pop(root.pageStack.currentItem)
            }
        })
    }
    pageStack.initialPage: StartPage {}

    // After a change of style (Appearance::rebuild) the pages of the old window, as keptPageList gave
    // them. A page names itself with `keptPage` (and `keptState`, its properties); only pages
    // without an unsaved input of their own do, and those are the only ways to the style choice.
    property var keptPages: []
    function keptPageList() {
        // A field in focus saves when it is left.
        root.contentItem.forceActiveFocus()
        const pages = []
        for (let i = 1; i <= root.pageStack.currentIndex; i++) {
            const page = root.pageStack.get(i)
            const name = page ? page["keptPage"] : undefined
            if (!name) {
                break
            }
            pages.push({ page: name, state: page["keptState"] ?? {}, contentY: page.flickable ? page.flickable.contentY : 0 })
        }
        return pages
    }
    // At once, without the slide: the window opens as it was.
    function restorePages() {
        const duration = root.pageStack.columnView.scrollDuration
        root.pageStack.columnView.scrollDuration = 0
        for (const kept of root.keptPages) {
            const page = root.pageStack.push(Qt.resolvedUrl(kept.page), kept.state)
            if (page.flickable && kept.contentY > 0) {
                root.scrollTo(page.flickable, kept.contentY)
            }
        }
        Qt.callLater(() => { root.pageStack.columnView.scrollDuration = duration })
    }

    // The old place, once the page is that tall: the previews on it come a moment later.
    function scrollTo(flickable, y) {
        const place = () => {
            if (flickable.contentHeight - flickable.height < y) {
                return
            }
            flickable.contentHeightChanged.disconnect(place)
            flickable.contentY = y
        }
        flickable.contentHeightChanged.connect(place)
        Qt.callLater(place)
    }

    // The first start: the wizard over the start menu (it goes back there).
    Component.onCompleted: {
        Appearance.adopt(root)
        restorePages()
        if (!Appearance.welcomed) {
            pageStack.push(Qt.resolvedUrl("WelcomePage.qml"))
        }
    }

    // Android's bars in the window's ground.
    Binding {
        target: Appearance
        property: "barColor"
        value: Look.backgroundColor
    }

    // The sync: after it brought something the services read anew; a change asks for one soon.
    Connections {
        target: Sync
        function onMerged() {
            Practice.refresh()
            Courses.refresh()
        }
    }
    Connections {
        target: Practice
        function onChanged() { Sync.touched() }
        function onNotesChanged() { Sync.touched() }
    }
    Connections {
        target: Courses
        function onChanged() { Sync.touched() }
    }
}
