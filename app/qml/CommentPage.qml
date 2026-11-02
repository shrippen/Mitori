import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante

// Comment of a Shinki Toho entry (texts of shinkiryu.de, shipped with the app), a reading column.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    required property var entry

    title: entry.comment ? entry.comment.titel : entry.title

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 36
            text: page.entry.comment ? page.entry.comment.text : ""
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("Source: shinkiryu.de, Itto-den Shinki Toho")
            color: KanteStyle.disabledTextColor
            font.pointSize: Kirigami.Theme.smallFont.pointSize
        }
    }
}
