import QtQuick
import org.kde.kirigami as Kirigami
import Kante
import Mitori

Kirigami.ApplicationWindow {
    id: root

    title: "Mitori"
    width: KanteStyle.unit(420)
    height: KanteStyle.unit(800)

    // The style Mitori started with (Appearance): Breeze and Material are the platform's
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
    // The first start: the wizard over the start menu (it goes back there).
    Component.onCompleted: {
        if (!Appearance.welcomed) {
            pageStack.push(Qt.resolvedUrl("WelcomePage.qml"))
        }
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
