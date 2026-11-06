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
//   CODE OF THE DVD   [7KQ2M-X9HDV-3TAPR-WE6YT] [Unlock] [Scan code]
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

    // A scan does what typing does: the code goes into the field, Unlock runs.
    Connections {
        target: CodeScanner

        function onReady() {
            // A second answer to one wish finds the scan page already on top.
            if (!page.isCurrentPage) {
                return
            }
            page.Kirigami.PageStack.push(Qt.resolvedUrl("ScanPage.qml"), { done: code => page.redeem(code) })
        }
        function onRefused(reason) {
            page.codeOk = false
            page.codeMessage = reason === CodeScanner.Refusal.NoCamera ? qsTr("This device has no camera. Type the code instead.")
                             : qsTr("Mitori may not use the camera. Type the code instead, or allow it in the system settings.")
        }
    }
    function redeem(code) {
        page.codeMessage = ""
        codeField.text = code
        unlockButton.clicked()
    }

    // The rows' model: a change of the list (progress, state) changes a row in place, so a press
    // on its button meets the same button when it is let go.
    KeyedListModel {
        id: bundleModel

        rows: Bundles.bundles
    }

    // Over a thousand MB in GB (the free space of a disk).
    function amount(bytes) {
        return bytes < 1e9 ? megabytes(bytes) : (bytes / 1e9).toLocaleString(Qt.locale(), 'f', 1) + " GB"
    }
    // Photos and voice notes change on other pages: read when this one opens.
    readonly property real mediaSize: Media.size()
    readonly property real storageTotal: Bundles.size + mediaSize + Bundles.freeSpace
    function megabytes(bytes) {
        return Math.round(bytes / 1e6).toLocaleString(Qt.locale(), 'f', 0) + " MB"
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        LookLabel {
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
        // In one row where the code fits beside the buttons; on a phone they go below it.
        GridLayout {
            id: codeRow

            readonly property bool narrow: page.width < Kirigami.Units.gridUnit * 26

            Layout.fillWidth: true
            columns: narrow ? 2 : 3

            LookTextField {
                id: codeField

                objectName: "codeField"
                Layout.fillWidth: true
                Layout.columnSpan: codeRow.narrow ? 2 : 1
                placeholderText: "XXXXX-XXXXX-…"
                font: Look.monoFont(Look.defaultFont.pointSize)
                inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                Accessible.name: qsTr("Code of the DVD")
                onAccepted: unlockButton.clicked()
                onTextEdited: page.codeMessage = ""
            }
            LookButton {
                id: unlockButton

                objectName: "unlockButton"
                Layout.fillWidth: codeRow.narrow
                role: LookButton.Role.Primary
                enabled: codeField.text.trim() !== "" && !Bundles.unlocking
                text: Bundles.unlocking ? qsTr("Checking …") : qsTr("Unlock")
                onClicked: Bundles.unlock(codeField.text)
            }
            LookButton {
                objectName: "scanButton"
                Layout.fillWidth: codeRow.narrow
                enabled: !Bundles.unlocking
                icon.name: "view-barcode-qr"
                text: qsTr("Scan code")
                Accessible.name: qsTr("Scan the code of the DVD")
                onClicked: CodeScanner.open()
            }
        }
        LookLabel {
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
        LookLabel {
            Layout.fillWidth: true
            text: qsTr("A bundle has 70–600 MB.")
            color: Look.mutedTextColor
            font: Look.smallFont
            wrapMode: Text.Wrap
        }
        LookLabel {
            Layout.fillWidth: true
            text: Bundles.manifestState === Bundles.ManifestState.Loading ? qsTr("Loading the list of videos …")
                : Bundles.manifestState === Bundles.ManifestState.Failed && Bundles.bundles.length === 0
                  ? qsTr("The list of videos could not be loaded. Reload later, or check the connection.")
                : qsTr("On this device: %1 of videos.").arg(page.megabytes(Bundles.size))
            color: Look.mutedTextColor
            wrapMode: Text.Wrap
        }
        LookProgressBar {
            objectName: "storageBar"
            Layout.fillWidth: true
            visible: page.storageTotal > 0
            parts: [
                { value: Bundles.size / page.storageTotal, color: Look.levelColor(4), text: page.amount(Bundles.size) + " " + qsTr("Videos") },
                { value: page.mediaSize / page.storageTotal, color: Look.levelColor(2), text: page.amount(page.mediaSize) + " " + qsTr("Photos, voice notes") }
            ]
            Accessible.name: qsTr("Storage")
        }
        LookLegend {
            objectName: "storageLegend"
            Layout.fillWidth: true
            visible: page.storageTotal > 0
            model: [
                { color: Look.levelColor(4), value: page.amount(Bundles.size), text: qsTr("Videos") },
                { color: Look.levelColor(2), value: page.amount(page.mediaSize), text: qsTr("Photos, voice notes") },
                { color: Look.levelColor(0), value: page.amount(Bundles.freeSpace), text: qsTr("free") }
            ]
        }
        LookLabel {
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

                    LookLabel {
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
        LookLabel {
            Layout.fillWidth: true
            visible: Bundles.bundles.some(b => b.state === Bundles.State.Installed)
            text: qsTr("Removing frees the space; the code stays saved, so the videos load again without it.")
            color: Look.mutedTextColor
            wrapMode: Text.Wrap
        }
    }
}
