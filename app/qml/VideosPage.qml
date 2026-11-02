pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Videos (clickdummy "Videos", "Code"): the code of a DVD unlocks its bundle, which then loads
// onto the device; each bundle loads, updates and goes on its own. A removed bundle keeps its
// code. Downloads wait for Wi-Fi unless the user says otherwise.
//
//   CODE OF THE DVD   [7KQ2M-X9HDV-3TAPR-WE6YC] [Unlock]
//   Aikido · all Kyu   5 videos · 600 MB · version 2       loaded       [Remove]
//   Daitoryu · all Kyu 5 videos · 330 MB                   not unlocked
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    // The answer to the last code: "" or what went wrong.
    property string codeMessage: ""
    property bool codeOk: false

    title: qsTr("Videos")

    actions: [
        Kirigami.Action {
            icon.name: "view-refresh"
            text: qsTr("Reload")
            enabled: Bundles.manifestState !== Bundles.ManifestState.Loading
            onTriggered: Bundles.reload()
        }
    ]

    Component.onCompleted: Bundles.reload(Bundles.Reload.Older)

    Connections {
        target: Bundles

        function onUnlocked(id) {
            const b = Bundles.bundles.find(x => x.id === id)
            page.codeOk = true
            page.codeMessage = qsTr("Unlocked: %1. It loads now.").arg(b ? b.title : id)
            codeField.clear()
            Bundles.load(id)
        }
        function onUnlockFailed(reason) {
            page.codeOk = false
            page.codeMessage = reason === Bundles.UnlockError.Invalid ? qsTr("This is not a complete code; check it for typing errors.")
                             : reason === Bundles.UnlockError.NoManifest ? qsTr("The list of videos is not loaded yet. Check the connection and try again.")
                             : reason === Bundles.UnlockError.Unknown ? qsTr("This code is not known (or no longer valid).")
                             : qsTr("This code does not open its bundle.")
        }
    }

    function megabytes(bytes) {
        return Math.round(bytes / 1e6).toLocaleString(Qt.locale(), 'f', 0) + " MB"
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("The exam videos come with the DVDs of the Shinkiryu association: each DVD has a code that loads its videos onto this device.")
            wrapMode: Text.Wrap
            color: KanteStyle.mutedTextColor
        }

        KanteSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("Code of the DVD")
        }
        RowLayout {
            Layout.fillWidth: true

            KanteTextField {
                id: codeField

                objectName: "codeField"
                Layout.fillWidth: true
                placeholderText: "XXXXX-XXXXX-XXXXX-XXXXX"
                font: KanteStyle.monoFont(Kirigami.Theme.defaultFont.pointSize)
                inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                Accessible.name: qsTr("Code of the DVD")
                onAccepted: unlockButton.clicked()
                onTextEdited: page.codeMessage = ""
            }
            KanteButton {
                id: unlockButton

                objectName: "unlockButton"
                emphasis: KanteButton.Emphasis.Primary
                enabled: codeField.text.trim() !== "" && !Bundles.unlocking
                text: Bundles.unlocking ? qsTr("Checking …") : qsTr("Unlock")
                onClicked: Bundles.unlock(codeField.text)
            }
        }
        QQC2.Label {
            objectName: "codeMessage"
            Layout.fillWidth: true
            visible: text !== ""
            text: page.codeMessage
            color: page.codeOk ? KanteStyle.positiveTextColor : KanteStyle.negativeTextColor
            wrapMode: Text.Wrap
        }
        KanteButton {
            text: qsTr("Order on shinkiryu.de")
            icon.name: "internet-services"
            onClicked: Qt.openUrlExternally("https://www.shinkiryu.de/shinkiryu/de/DVD.php")
        }

        KanteSectionLabel {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            rule: true
            text: qsTr("Bundles")
        }
        KanteSwitch {
            objectName: "wifiOnly"
            Layout.fillWidth: true
            text: qsTr("Load only over Wi-Fi")
            checked: Bundles.wifiOnly
            onToggled: Bundles.wifiOnly = checked
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("A bundle has 70–600 MB.")
            color: KanteStyle.mutedTextColor
            font: KanteStyle.smallFont
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: Bundles.manifestState === Bundles.ManifestState.Loading ? qsTr("Loading the list of videos …")
                : Bundles.manifestState === Bundles.ManifestState.Failed && Bundles.bundles.length === 0
                  ? qsTr("The list of videos could not be loaded. Check the connection and reload.")
                : qsTr("On this device: %1 of videos.").arg(page.megabytes(Bundles.size))
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: Bundles.errorString !== ""
            text: Bundles.errorString
            color: KanteStyle.negativeTextColor
            wrapMode: Text.Wrap
        }

        Repeater {
            model: Bundles.bundles

            KanteListRow {
                id: row

                required property var modelData
                readonly property int bundleState: modelData.state

                objectName: "bundleRow"
                Layout.fillWidth: true
                text: modelData.title
                subtitle: [qsTr("%n video(s)", "", modelData.videos.length), page.megabytes(modelData.size),
                           modelData.installed > 0 ? qsTr("version %1").arg(modelData.installed) : "",
                           row.bundleState === Bundles.State.Update ? qsTr("version %1: %2").arg(modelData.version).arg(modelData.hint || qsTr("new")) : ""]
                          .filter(x => x).join(" · ")
                trailing: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        objectName: "bundleState"
                        text: row.bundleState === Bundles.State.Locked ? qsTr("not unlocked")
                            : row.bundleState === Bundles.State.Unlocked ? qsTr("code saved")
                            : row.bundleState === Bundles.State.Waiting ? qsTr("waits for Wi-Fi")
                            : row.bundleState === Bundles.State.Downloading ? qsTr("loading %1 %").arg(Math.floor(row.modelData.progress * 100))
                            : row.bundleState === Bundles.State.Installed ? qsTr("loaded")
                            : row.bundleState === Bundles.State.Update ? qsTr("update")
                            : qsTr("failed")
                        color: row.bundleState === Bundles.State.Failed ? KanteStyle.negativeTextColor : KanteStyle.mutedTextColor
                        font: KanteStyle.smallFont
                    }
                    KanteButton {
                        objectName: "bundleAction"
                        visible: row.bundleState !== Bundles.State.Locked
                        text: row.bundleState === Bundles.State.Downloading ? qsTr("Cancel")
                            : row.bundleState === Bundles.State.Installed ? qsTr("Remove")
                            : row.bundleState === Bundles.State.Update ? qsTr("Update")
                            : row.bundleState === Bundles.State.Waiting ? qsTr("Load now")
                            : qsTr("Load")
                        onClicked: {
                            const id = row.modelData.id
                            if (row.bundleState === Bundles.State.Downloading) {
                                Bundles.cancel(id)
                            } else if (row.bundleState === Bundles.State.Installed) {
                                Bundles.remove(id)
                            } else {
                                // "Load now" overrides Wi-Fi only once.
                                Bundles.load(id, row.bundleState === Bundles.State.Waiting ? Bundles.Network.Any : Bundles.Network.WifiOnly)
                            }
                        }
                    }
                }
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: Bundles.bundles.some(b => b.state === Bundles.State.Installed)
            text: qsTr("Removing frees the space; the code stays saved, so the videos load again without it.")
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }
    }
}
