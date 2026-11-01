import QtQuick
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Start menu, first step: the disciplines of the programme (layout: clickdummy, "Startmenü").
Kirigami.ScrollablePage {
    id: page

    title: "Mitori 見取り"

    ListView {
        model: Catalog.disciplines

        delegate: KanteListRow {
            required property var modelData

            width: ListView.view.width
            text: modelData.titel
            subtitle: modelData.jp
            meta: qsTr("%n grades", "", modelData.grade.length)
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: Catalog.errorString !== ""
            icon.name: "data-error"
            text: qsTr("Programme data missing")
            explanation: Catalog.errorString
        }
    }
}
