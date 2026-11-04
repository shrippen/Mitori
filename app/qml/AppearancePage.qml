import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// Settings → Appearance: the style and the language (both apply at the next start), the
// introduction again, and about Mitori (version, programme data, sources).
Kirigami.ScrollablePage {
    id: page

    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    title: qsTr("Appearance")

    // The languages in the order of Appearance.Language.
    readonly property var languages: [qsTr("System"), "Deutsch", "English"]

    ColumnLayout {
        spacing: Look.unit(12)

        LookSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("Style")
        }
        StyleChoice {
            Layout.fillWidth: true
        }

        LookSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("Language")
        }
        LookSegmented {
            objectName: "languageChoice"
            Layout.fillWidth: true
            model: page.languages
            currentIndex: Appearance.language
            onActivated: index => Appearance.setLanguage(index)
        }

        LookHint {
            objectName: "restartHint"
            Layout.fillWidth: true
            visible: Appearance.restartNeeded
            titleText: qsTr("At the next start")
            text: qsTr("Style and language change when Mitori starts again.")
        }

        LookButton {
            objectName: "introButton"
            icon.name: "documentinfo"
            text: qsTr("Show the introduction")
            onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("WelcomePage.qml"))
        }

        LookSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("About Mitori")
        }
        LookLabel {
            objectName: "aboutText"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTr("Mitori %1. The exam programmes of Aikido Shinki Rengo, Daitoryu and Itto-den Shinki Toho, with the exam videos of the DVDs.").arg(Appearance.version)
        }
        Repeater {
            model: Programme.disciplines

            LookLabel {
                required property var modelData

                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: Look.mutedTextColor
                // The programme's date as the data has it (German, as the programmes).
                text: [modelData.titel, modelData.stand].filter(x => !!x).join(" · ")
            }
        }
        LookLabel {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Look.mutedTextColor
            text: qsTr("Sources: the exam regulations and lists of the dojo, the comments of shinkiryu.de and the Aiki-Budo lexicon.")
        }
    }
}
