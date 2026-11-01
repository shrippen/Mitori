import QtQuick
import org.kde.kirigami as Kirigami
import Kante

Kirigami.ApplicationWindow {
    id: root

    title: "Mitori"
    width: KanteStyle.unit(420)
    height: KanteStyle.unit(800)

    Binding {
        target: KanteStyle
        property: "kind"
        value: KanteStyle.Kind.Kante
    }
    KanteScope { target: root.contentItem }

    // One page at a time, also in landscape: the tablet gets its own page with list and player
    // (app-entscheidungen.md, "Tablet quer"), not Kirigami's columns.
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    pageStack.initialPage: StartPage {}
}
