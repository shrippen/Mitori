import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// Settings: the video bundles (codes of the DVDs), a folder with exam videos of one's own, and
// the backup (save everything as a file, read one back: merged or replacing, see Backup), and the sync
// between devices over WebDAV / Nextcloud (Sync).
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    title: qsTr("Settings")

    // The settings as rows of its own: the name of the setting as a small caps line above its
    // field (the clickdummy's forms), not the desktop's form layout.
    ColumnLayout {
        spacing: Look.unit(16)

        LookFormRow {
            label: qsTr("Appearance")
            LookButton {
                objectName: "appearanceButton"
                icon.name: "configure"
                text: qsTr("Style, language, about Mitori")
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("AppearancePage.qml"))
            }
        }
        LookFormRow {
            label: qsTr("Exam videos")
            LookButton {
                objectName: "videosButton"
                icon.name: "media-playback-start"
                text: qsTr("Videos and DVD codes")
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
            }
        }
        LookFormRow {
            label: qsTr("Video folder")
            LookTextField {
                id: folderField

                Layout.fillWidth: true
                text: Videos.folder
                onEditingFinished: Videos.folder = text
            }
        }
        LookLabel {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            wrapMode: Text.Wrap
            color: Look.disabledTextColor
            text: qsTr("Without a bundle: a folder with the videos, folders and names as on the DVDs: Aikido/5. Kyu.mkv, Daitoryu/1. Dan.mkv, Shinki Toho/Shoden.mkv …")
                  + (Videos.sandboxed ? " " + qsTr("The Flatpak can read folders under ~/Videos only.") : "")
        }

        LookSectionLabel {
            objectName: "backupSection"
            text: qsTr("Backup")
        }
        LookFormRow {
            LookButton {
                objectName: "saveBackup"
                icon.name: "document-save"
                text: qsTr("Save backup …")
                onClicked: {
                    saveDialog.selectedFile = saveDialog.currentFolder + "/" + Backup.fileName()
                    saveDialog.open()
                }
            }
        }
        LookButton {
            objectName: "readBackup"
            icon.name: "document-open"
            text: qsTr("Read backup …")
            onClicked: openDialog.open()
        }
        LookLabel {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            wrapMode: Text.Wrap
            color: Look.disabledTextColor
            text: qsTr("One file with your ratings, notes, exam rounds, notebooks, grades, photos and voice notes. Reading one back asks: merge it (what is newer stays, nothing gets lost) or replace this device's data with it. DVD codes are not in it.")
        }
        LookLabel {
            objectName: "backupDone"
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            visible: page.done !== ""
            text: page.done
            color: Look.positiveTextColor
            wrapMode: Text.Wrap
        }
        LookLabel {
            objectName: "backupError"
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            visible: Backup.errorString !== ""
            text: Backup.errorString
            color: Look.negativeTextColor
            wrapMode: Text.Wrap
        }

        // Sync over WebDAV / Nextcloud (Sync): account, state, devices, conflicts.
        LookSectionLabel {
            objectName: "syncSection"
            text: qsTr("Sync between devices")
        }
        LookFormRow {
            label: qsTr("WebDAV / Nextcloud")
            LookSwitch {
                objectName: "syncEnabled"
                text: qsTr("Sync")
                checked: Sync.enabled
                onToggled: Sync.enabled = checked
            }
        }
        LookFormRow {
            label: qsTr("Folder on the server")
            visible: Sync.enabled
            LookTextField {
                id: serverField

                objectName: "syncServer"
                Layout.fillWidth: true
                text: Sync.server
                placeholderText: "https://cloud.example.org/remote.php/dav/files/ich/Mitori/"
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
                onEditingFinished: page.saveAccount()
            }
        }
        LookFormRow {
            label: qsTr("User")
            visible: Sync.enabled
            LookTextField {
                id: userField

                Layout.fillWidth: true
                text: Sync.user
                inputMethodHints: Qt.ImhNoAutoUppercase
                onEditingFinished: page.saveAccount()
            }
        }
        LookFormRow {
            label: qsTr("App password")
            visible: Sync.enabled
            LookTextField {
                id: passwordField

                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: Sync.hasPassword ? qsTr("saved") : ""
                onEditingFinished: page.saveAccount()
            }
        }
        LookFormRow {
            label: qsTr("This device")
            visible: Sync.enabled
            LookTextField {
                id: nameField

                Layout.fillWidth: true
                text: Sync.deviceName
                onEditingFinished: page.saveAccount()
            }
        }
        LookSwitch {
            visible: Sync.enabled
            text: qsTr("Sync after every change")
            checked: Sync.automatic
            onToggled: Sync.automatic = checked
        }
        LookLabel {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            visible: Sync.enabled
            wrapMode: Text.Wrap
            color: Look.disabledTextColor
            text: qsTr("Otherwise when Mitori starts and once a day. Every rating and note is an entry of its own with a time; changes from several devices are merged. Mitori asks only when the same note was changed on two devices.")
        }
        LookButton {
            objectName: "syncNow"
            visible: Sync.enabled
            enabled: !Sync.busy
            icon.name: "view-refresh"
            text: Sync.busy ? qsTr("Syncing …") : qsTr("Sync now")
            onClicked: {
                page.saveAccount()
                Sync.syncNow()
            }
        }
        LookLabel {
            objectName: "syncState"
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            visible: Sync.enabled
            wrapMode: Text.Wrap
            color: Sync.errorString !== "" ? Look.negativeTextColor : Look.textColor
            text: Sync.errorString !== "" ? Sync.errorString
                  : Sync.lastSync.getTime() > 0 ? qsTr("Last synced: %1").arg(Sync.lastSync.toLocaleString(Qt.locale(), Locale.ShortFormat))
                  : qsTr("Not synced yet.")
        }
        Repeater {
            model: Sync.enabled ? Sync.devices : []

            LookLabel {
                required property var modelData

                text: qsTr("%1: %2").arg(modelData.name || qsTr("Device")).arg(modelData.synced.toLocaleString(Qt.locale(), Locale.ShortFormat))
                color: Look.disabledTextColor
            }
        }
        LookButton {
            objectName: "syncConflicts"
            visible: Sync.enabled && Sync.conflicts.length > 0
            icon.name: "dialog-warning"
            text: qsTr("%n conflict(s): solve …", "", Sync.conflicts.length)
            onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("ConflictsPage.qml"))
        }
    }

    function saveAccount() {
        Sync.setAccount(serverField.text, userField.text, passwordField.text, nameField.text)
        passwordField.text = ""
    }

    // What the last step did ("" for nothing yet), and what the read backup would change.
    property string done: ""
    property var summary: ({})

    FileDialog {
        id: saveDialog

        title: qsTr("Save backup")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "zip"
        nameFilters: [qsTr("ZIP archives (*.zip)")]
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        onAccepted: page.done = Backup.save(selectedFile)
                                ? (Backup.savedTo() !== "" ? qsTr("Backup saved: %1").arg(Backup.savedTo()) : qsTr("Backup saved."))
                                : ""
    }

    FileDialog {
        id: openDialog

        title: qsTr("Read backup")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("ZIP archives (*.zip)")]
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        onAccepted: page.inspect(selectedFile)
    }

    function inspect(url) {
        page.done = ""
        page.summary = Backup.inspect(url)
        if (Object.keys(page.summary).length > 0) {
            mergeDialog.open()
        }
    }

    // "Notes: 3 new, 2 changed" per kind that changes; a line when nothing does.
    function changes(s) {
        const kinds = [["ratings", qsTr("Ratings")], ["notes", qsTr("Notes")], ["rounds", qsTr("Exam rounds")],
                       ["notebooks", qsTr("Notebooks")], ["entries", qsTr("Notebook entries")], ["grades", qsTr("Grades")],
                       ["starts", qsTr("Start of training")]]
        let lines = []
        for (const [key, name] of kinds) {
            const c = s[key] || {}
            let parts = []
            if (c.added > 0) {
                parts.push(qsTr("%n new", "", c.added))
            }
            if (c.changed > 0) {
                parts.push(qsTr("%n changed", "", c.changed))
            }
            if (parts.length > 0) {
                lines.push(name + ": " + parts.join(", "))
            }
        }
        if (s.media > 0) {
            lines.push(qsTr("Photos and voice notes: %n new", "", s.media))
        }
        return lines.length > 0 ? lines.join("\n") : qsTr("Nothing new: this device has everything in the backup.")
    }

    // "Ratings: 12" per kind, by the summary's key for that number; the media's number apart.
    function amounts(s, key, media) {
        const kinds = [["ratings", qsTr("Ratings")], ["notes", qsTr("Notes")], ["rounds", qsTr("Exam rounds")],
                       ["notebooks", qsTr("Notebooks")], ["entries", qsTr("Notebook entries")], ["grades", qsTr("Grades")],
                       ["starts", qsTr("Start of training")]]
        let lines = []
        for (const [kind, name] of kinds) {
            const n = (s[kind] || {})[key] || 0
            if (n > 0) {
                lines.push(name + ": " + n)
            }
        }
        if (media > 0) {
            lines.push(qsTr("Photos and voice notes") + ": " + media)
        }
        return lines
    }

    // How much of the window's height a dialog may take (as Kante's size steps).
    readonly property real dialogRoom: 0.9

    // What a replace means for the sync; "" without an account.
    function syncNote() {
        if (Sync.enabled) {
            return qsTr("Sync is turned off on this device: your other devices keep their data and would bring it back here. If you turn sync on again, what they hold comes back.")
        }
        return Sync.server !== "" ? qsTr("If you turn sync on again, what your other devices hold comes back here.") : ""
    }

    LookDialog {
        id: mergeDialog

        objectName: "mergeBackup"
        parent: QQC2.Overlay.overlay
        anchors.centerIn: parent
        modal: true
        span: LookDialog.Span.Small
        // Taller than a phone held sideways: the text scrolls, the buttons stay.
        height: Math.min(implicitHeight, (parent ? parent.height : implicitHeight) * page.dialogRoom)
        title: qsTr("Read backup")
        acceptText: qsTr("Merge")
        standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
        onAccepted: page.done = Backup.restore() ? qsTr("Backup merged.") : ""
        onRejected: Backup.discard()

        LookScrollView {
            id: mergeScroll

            objectName: "mergeScroll"
            anchors.fill: parent
            contentWidth: availableWidth
            rightPadding: Look.unit(8)      // the bar stands beside the text
            clip: true

            ColumnLayout {
                width: mergeScroll.availableWidth
                spacing: Look.unit(16)

                LookLabel {
                    id: mergeText

                    objectName: "mergeText"
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    // Closed it measures nothing: a window turning (full screen) made a page still on its way
                    // out wrap this text anew, a binding loop in the dialog's height.
                    text: !mergeDialog.visible ? "" : (page.summary.created ? qsTr("Backup of %1.").arg(page.summary.created.toLocaleString(Qt.locale(), Locale.ShortFormat)) + "\n\n" : "")
                          + qsTr("It holds: %1").arg(page.amounts(page.summary, "holds", page.summary.mediaHolds).join(", ") || qsTr("nothing"))
                          + "\n\n" + qsTr("Merging keeps everything on this device and takes in:") + "\n"
                          + page.changes(page.summary)
                }
                LookLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: !mergeDialog.visible ? "" : qsTr("Replacing deletes what is on this device and puts the backup's state in its place. It asks once more.")
                }
                LookButton {
                    objectName: "replaceInstead"
                    icon.name: "edit-delete"
                    text: qsTr("Replace instead …")
                    onClicked: {
                        mergeDialog.close()
                        replaceDialog.open()
                    }
                }
            }
        }
    }

    // The second step of a replace: says what goes. Cancel goes back to the first.
    LookDialog {
        id: replaceDialog

        objectName: "replaceBackup"
        parent: QQC2.Overlay.overlay
        anchors.centerIn: parent
        modal: true
        destructive: true
        span: LookDialog.Span.Small
        height: Math.min(implicitHeight, (parent ? parent.height : implicitHeight) * page.dialogRoom)
        title: qsTr("Replace this device's data?")
        acceptText: qsTr("Delete and replace")
        standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
        onAccepted: {
            // A running sync would merge into the new state: wait for it (the note below says so).
            if (Sync.busy) {
                replaceDialog.open()
                return
            }
            const ok = Backup.replace()
            if (ok) {
                Sync.dataReplaced()
            }
            page.done = ok ? qsTr("This device's data was replaced by the backup.") : ""
        }
        onRejected: mergeDialog.open()

        LookScrollView {
            id: replaceScroll

            objectName: "replaceScroll"
            anchors.fill: parent
            contentWidth: availableWidth
            rightPadding: Look.unit(8)      // the bar stands beside the text
            clip: true

            LookLabel {
                objectName: "replaceText"
                width: replaceScroll.availableWidth
                wrapMode: Text.Wrap
                text: {
                    if (!replaceDialog.visible) {
                        return ""
                    }
                    const lost = page.amounts(page.summary, "lost", page.summary.mediaLost)
                    return qsTr("This cannot be undone. The backup's state takes the place of this device's ratings, notes, exam rounds, notebooks, grades, photos and voice notes.")
                           + "\n\n" + (lost.length > 0 ? qsTr("Deleted, or set back to the backup's older state:") + "\n" + lost.join("\n")
                                                        : qsTr("The backup has all of this device's data, none is lost."))
                           + "\n\n" + qsTr("Downloaded videos, DVD codes and settings stay.")
                           + (page.syncNote() !== "" ? "\n\n" + page.syncNote() : "")
                           + (Sync.busy ? "\n\n" + qsTr("A sync is running: replace when it has finished.") : "")
                }
            }
        }
    }

    // The services read their data anew after a merge or a replace.
    Connections {
        target: Backup
        function onRestored() {
            Practice.refresh()
            Courses.refresh()
        }
    }
}
