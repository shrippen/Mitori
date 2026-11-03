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
    // colours, Kante dark, Leinen light. Shinki stands in Leinen until app/qml/Look reaches it.
    readonly property bool platformStyle: Appearance.running === Appearance.Style.Breeze
                                          || Appearance.running === Appearance.Style.Material
    Binding {
        target: KanteStyle
        property: "kind"
        value: root.platformStyle ? KanteStyle.Kind.System : KanteStyle.Kind.Kante
    }
    Binding {
        target: KanteStyle
        property: "preferDark"
        value: Appearance.running === Appearance.Style.Kante
    }
    Binding {
        target: KanteStyle
        property: "preferLight"
        value: Appearance.running === Appearance.Style.Leinen || Appearance.running === Appearance.Style.Shinki
    }
    KanteScope { target: root.contentItem }

    // One page at a time, also in landscape: the tablet gets its own page with list and player
    // (app-entscheidungen.md, "Tablet quer"), not Kirigami's columns.
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    // Under Shinki the window carries Shinki's head in place of Kirigami's bar (bible section 5,
    // Kopf); the other styles keep the bar (LookHead is empty there).
    pageStack.globalToolBar.style: Look.shinki ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.Auto
    header: LookHead {
        objectName: "lookHead"
        page: root.pageStack.currentItem
        backTitle: root.pageStack.depth > 1 && root.pageStack.get(root.pageStack.depth - 2)
                   ? root.pageStack.get(root.pageStack.depth - 2).title || "" : ""
        moreText: qsTr("More")
        onBack: root.pageStack.pop()
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
