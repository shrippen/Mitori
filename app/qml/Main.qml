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

    pageStack.initialPage: StartPage {}
}
