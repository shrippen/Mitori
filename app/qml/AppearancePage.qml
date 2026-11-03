import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Settings → Appearance: the style and the language (both apply at the next start), the
// introduction again, and about Mitori (version, programme data, sources).
Kirigami.ScrollablePage {
    id: page

    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    title: qsTr("Appearance")

    // The languages in the order of Appearance.Language.
    readonly property var languages: [qsTr("System"), "Deutsch", "English"]

    ColumnLayout {
        spacing: KanteStyle.unit(12)

        KanteSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("Style")
        }
        StyleChoice {
            Layout.fillWidth: true
        }

        KanteSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("Language")
        }
        KanteSegmented {
            objectName: "languageChoice"
            Layout.fillWidth: true
            model: page.languages
            currentIndex: Appearance.language
            onActivated: index => Appearance.setLanguage(index)
        }

        KanteHintCard {
            objectName: "restartHint"
            Layout.fillWidth: true
            visible: Appearance.restartNeeded
            titleText: qsTr("At the next start")
            text: qsTr("Style and language change when Mitori starts again.")
        }

        KanteButton {
            objectName: "introButton"
            icon.name: "documentinfo"
            text: qsTr("Show the introduction")
            onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("WelcomePage.qml"))
        }

        KanteSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("About Mitori")
        }
        QQC2.Label {
            objectName: "aboutText"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Mitori %1. The exam programmes of Aikido Shinki Rengo, Daitoryu and Itto-den Shinki Toho, with the exam videos of the DVDs.").arg(Appearance.version)
        }
        Repeater {
            model: Programme.disciplines

            QQC2.Label {
                required property var modelData

                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: KanteStyle.mutedTextColor
                text: modelData.titel + " · " + (modelData.stand || "")
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: KanteStyle.mutedTextColor
            text: qsTr("Sources: the exam regulations and lists of the dojo, the comments of shinkiryu.de and the Aiki-Budo lexicon.")
        }
    }
}
