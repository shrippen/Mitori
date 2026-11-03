pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
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

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

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
            page.codeMessage = reason === Bundles.UnlockError.Invalid ? qsTr("A code has 20 characters (letters and digits); check that none is missing or extra.")
                             : reason === Bundles.UnlockError.Typo ? qsTr("This code has a typing error; check it character by character.")
                             : reason === Bundles.UnlockError.NoManifest ? qsTr("The list of videos is not loaded yet. Check the connection and try again.")
                             : reason === Bundles.UnlockError.Unknown ? qsTr("This code is not known (or no longer valid).")
                             : qsTr("This code does not open its bundle.")
        }
    }

    // The rows' model: a change of the list (progress, state) changes a row in place, so a press
    // on its button meets the same button when it is let go.
    KeyedListModel {
        id: bundleModel

        rows: Bundles.bundles
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
            color: Look.mutedTextColor
        }

        LookSectionLabel {
            Layout.fillWidth: true
            rule: true
            text: qsTr("Code of the DVD")
        }
        RowLayout {
            Layout.fillWidth: true

            LookTextField {
                id: codeField

                objectName: "codeField"
                Layout.fillWidth: true
                placeholderText: "XXXXX-XXXXX-XXXXX-XXXXX"
                font: Look.monoFont(Kirigami.Theme.defaultFont.pointSize)
                inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                Accessible.name: qsTr("Code of the DVD")
                onAccepted: unlockButton.clicked()
                onTextEdited: page.codeMessage = ""
            }
            LookButton {
                id: unlockButton

                objectName: "unlockButton"
                role: LookButton.Role.Primary
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
            color: page.codeOk ? Look.positiveTextColor : Look.negativeTextColor
            wrapMode: Text.Wrap
        }
        LookButton {
            text: qsTr("Order on shinkiryu.de")
            icon.name: "internet-services"
            onClicked: Qt.openUrlExternally("https://www.shinkiryu.de/shinkiryu/de/DVD.php")
        }

        LookSectionLabel {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            rule: true
            text: qsTr("Bundles")
        }
        LookSwitch {
            objectName: "wifiOnly"
            Layout.fillWidth: true
            text: qsTr("Load only over Wi-Fi")
            checked: Bundles.wifiOnly
            onToggled: Bundles.wifiOnly = checked
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("A bundle has 70–600 MB.")
            color: Look.mutedTextColor
            font: Look.smallFont
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: Bundles.manifestState === Bundles.ManifestState.Loading ? qsTr("Loading the list of videos …")
                : Bundles.manifestState === Bundles.ManifestState.Failed && Bundles.bundles.length === 0
                  ? qsTr("The list of videos could not be loaded. Check the connection and reload.")
                : qsTr("On this device: %1 of videos.").arg(page.megabytes(Bundles.size))
            color: Look.mutedTextColor
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: Bundles.errorString !== ""
            text: Bundles.errorString
            color: Look.negativeTextColor
            wrapMode: Text.Wrap
        }

        Repeater {
            model: bundleModel

            LookListRow {
                id: bundleRow

                required property var row
                readonly property int bundleState: row.state

                objectName: "bundleRow"
                Layout.fillWidth: true
                text: row.title
                subtitle: [qsTr("%n video(s)", "", row.videos.length), page.megabytes(row.size),
                           row.installed > 0 ? qsTr("version %1").arg(row.installed) : "",
                           bundleRow.bundleState === Bundles.State.Update ? qsTr("version %1: %2").arg(row.version).arg(row.hint || qsTr("new")) : ""]
                          .filter(x => x).join(" · ")
                trailing: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        objectName: "bundleState"
                        text: bundleRow.bundleState === Bundles.State.Locked ? qsTr("not unlocked")
                            : bundleRow.bundleState === Bundles.State.Unlocked ? qsTr("code saved")
                            : bundleRow.bundleState === Bundles.State.Waiting ? qsTr("waits for Wi-Fi")
                            : bundleRow.bundleState === Bundles.State.Downloading ? qsTr("loading %1 %").arg(Math.floor(bundleRow.row.progress * 100))
                            : bundleRow.bundleState === Bundles.State.Installed ? qsTr("loaded")
                            : bundleRow.bundleState === Bundles.State.Update ? qsTr("update")
                            : qsTr("failed")
                        color: bundleRow.bundleState === Bundles.State.Failed ? Look.negativeTextColor : Look.mutedTextColor
                        font: Look.smallFont
                    }
                    LookButton {
                        objectName: "bundleAction"
                        visible: bundleRow.bundleState !== Bundles.State.Locked
                        text: bundleRow.bundleState === Bundles.State.Downloading ? qsTr("Cancel")
                            : bundleRow.bundleState === Bundles.State.Installed ? qsTr("Remove")
                            : bundleRow.bundleState === Bundles.State.Update ? qsTr("Update")
                            : bundleRow.bundleState === Bundles.State.Waiting ? qsTr("Load now")
                            : qsTr("Load")
                        onClicked: {
                            const id = bundleRow.row.id
                            if (bundleRow.bundleState === Bundles.State.Downloading) {
                                Bundles.cancel(id)
                            } else if (bundleRow.bundleState === Bundles.State.Installed) {
                                Bundles.remove(id)
                            } else {
                                // "Load now" overrides Wi-Fi only once.
                                Bundles.load(id, bundleRow.bundleState === Bundles.State.Waiting ? Bundles.Network.Any : Bundles.Network.WifiOnly)
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
            color: Look.mutedTextColor
            wrapMode: Text.Wrap
        }
    }
}
