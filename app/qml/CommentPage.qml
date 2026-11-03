pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import "comment.js" as Comment

// Comment of a Shinki Toho entry (texts of shinkiryu.de, shipped with the app), a reading column.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    required property var entry

    title: entry.comment ? entry.comment.titel : entry.title

    // The column's measure: about 70 letters, easy to read.
    readonly property real measure: Kirigami.Units.gridUnit * 36

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        // The whole title (the page header elides a long one: "Ichi-ban/ Ippon-me: …").
        KanteHeading {
            objectName: "commentTitle"
            Layout.fillWidth: true
            Layout.maximumWidth: page.measure
            level: 2
            wrapMode: Text.Wrap
            text: page.title
        }
        // Paragraphs and headings of the text (comment.js).
        Repeater {
            model: Comment.blocks(page.entry.comment ? page.entry.comment.text : "")

            Loader {
                id: block

                required property var modelData

                Layout.fillWidth: true
                Layout.maximumWidth: page.measure
                sourceComponent: block.modelData.heading ? headingBlock : paragraphBlock

                Component {
                    id: headingBlock

                    KanteSectionLabel {
                        objectName: "commentHeading"
                        rule: true
                        text: block.modelData.text
                    }
                }
                Component {
                    id: paragraphBlock

                    QQC2.Label {
                        objectName: "commentParagraph"
                        text: block.modelData.text
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("Source: shinkiryu.de, Itto-den Shinki Toho")
            color: KanteStyle.disabledTextColor
            font.pointSize: Kirigami.Theme.smallFont.pointSize
        }
    }
}
