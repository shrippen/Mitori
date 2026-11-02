import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Settings: the video bundles (codes of the DVDs), a folder with exam videos of one's own, and
// the backup (save everything as a file, read one back: it is merged, see Backup), and the sync
// between devices over WebDAV / Nextcloud (Sync).
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    title: qsTr("Settings")

    Kirigami.FormLayout {
        KanteButton {
            objectName: "videosButton"
            Kirigami.FormData.label: qsTr("Exam videos:")
            icon.name: "media-playback-start"
            text: qsTr("Videos and DVD codes")
            onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
        }
        KanteTextField {
            id: folderField

            Kirigami.FormData.label: qsTr("Video folder:")
            Layout.fillWidth: true
            text: Videos.folder
            onEditingFinished: Videos.folder = text
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            wrapMode: Text.Wrap
            color: KanteStyle.disabledTextColor
            text: qsTr("Without a bundle: a folder with the videos, folders and names as on the DVDs: Aikido/5. Kyu.mkv, Daitoryu/1. Dan.mkv, Shinki Toho/Shoden.mkv …")
                  + (Videos.sandboxed ? " " + qsTr("The Flatpak can read folders under ~/Videos only.") : "")
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: qsTr("Backup")
        }
        KanteButton {
            objectName: "saveBackup"
            Kirigami.FormData.label: qsTr("Backup:")
            icon.name: "document-save"
            text: qsTr("Save backup …")
            onClicked: {
                saveDialog.selectedFile = saveDialog.currentFolder + "/" + Backup.fileName()
                saveDialog.open()
            }
        }
        KanteButton {
            objectName: "readBackup"
            icon.name: "document-open"
            text: qsTr("Read backup …")
            onClicked: openDialog.open()
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            wrapMode: Text.Wrap
            color: KanteStyle.disabledTextColor
            text: qsTr("One file with your ratings, notes, exam rounds, notebooks, grades, photos and voice notes. Reading one back merges it: what is newer stays, nothing gets lost. DVD codes are not in it.")
        }
        QQC2.Label {
            objectName: "backupDone"
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            visible: page.done !== ""
            text: page.done
            color: KanteStyle.positiveTextColor
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            objectName: "backupError"
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            visible: Backup.errorString !== ""
            text: Backup.errorString
            color: KanteStyle.negativeTextColor
            wrapMode: Text.Wrap
        }

        // Sync over WebDAV / Nextcloud (Sync): account, state, devices, conflicts.
        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: qsTr("Sync between devices")
        }
        KanteSwitch {
            objectName: "syncEnabled"
            Kirigami.FormData.label: qsTr("WebDAV / Nextcloud:")
            text: qsTr("Sync")
            checked: Sync.enabled
            onToggled: Sync.enabled = checked
        }
        KanteTextField {
            id: serverField

            objectName: "syncServer"
            Kirigami.FormData.label: qsTr("Folder on the server:")
            Layout.fillWidth: true
            visible: Sync.enabled
            text: Sync.server
            placeholderText: "https://cloud.example.org/remote.php/dav/files/ich/Mitori/"
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
            onEditingFinished: page.saveAccount()
        }
        KanteTextField {
            id: userField

            Kirigami.FormData.label: qsTr("User:")
            Layout.fillWidth: true
            visible: Sync.enabled
            text: Sync.user
            inputMethodHints: Qt.ImhNoAutoUppercase
            onEditingFinished: page.saveAccount()
        }
        KanteTextField {
            id: passwordField

            Kirigami.FormData.label: qsTr("App password:")
            Layout.fillWidth: true
            visible: Sync.enabled
            echoMode: TextInput.Password
            placeholderText: Sync.hasPassword ? qsTr("saved") : ""
            onEditingFinished: page.saveAccount()
        }
        KanteTextField {
            id: nameField

            Kirigami.FormData.label: qsTr("This device:")
            Layout.fillWidth: true
            visible: Sync.enabled
            text: Sync.deviceName
            onEditingFinished: page.saveAccount()
        }
        KanteSwitch {
            visible: Sync.enabled
            text: qsTr("Sync after every change")
            checked: Sync.automatic
            onToggled: Sync.automatic = checked
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            visible: Sync.enabled
            wrapMode: Text.Wrap
            color: KanteStyle.disabledTextColor
            text: qsTr("Otherwise when Mitori starts and once a day. Every rating and note is an entry of its own with a time; changes from several devices are merged. Mitori asks only when the same note was changed on two devices.")
        }
        KanteButton {
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
        QQC2.Label {
            objectName: "syncState"
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            visible: Sync.enabled
            wrapMode: Text.Wrap
            color: Sync.errorString !== "" ? KanteStyle.negativeTextColor : KanteStyle.textColor
            text: Sync.errorString !== "" ? Sync.errorString
                  : Sync.lastSync.getTime() > 0 ? qsTr("Last synced: %1").arg(Sync.lastSync.toLocaleString(Qt.locale(), Locale.ShortFormat))
                  : qsTr("Not synced yet.")
        }
        Repeater {
            model: Sync.enabled ? Sync.devices : []

            QQC2.Label {
                required property var modelData

                text: qsTr("%1: %2").arg(modelData.name || qsTr("Device")).arg(modelData.synced.toLocaleString(Qt.locale(), Locale.ShortFormat))
                color: KanteStyle.disabledTextColor
            }
        }
        KanteButton {
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
        onAccepted: page.done = Backup.save(selectedFile) ? qsTr("Backup saved.") : ""
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
                       ["notebooks", qsTr("Notebooks")], ["entries", qsTr("Notebook entries")], ["grades", qsTr("Grades")]]
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

    KanteDialog {
        id: mergeDialog

        objectName: "mergeBackup"
        parent: QQC2.Overlay.overlay
        anchors.centerIn: parent
        modal: true
        size: KanteDialog.Size.Small
        title: qsTr("Merge backup?")
        acceptText: qsTr("Merge")
        standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
        contentHeight: mergeText.implicitHeight
        onAccepted: page.done = Backup.restore() ? qsTr("Backup merged.") : ""
        onRejected: Backup.discard()

        QQC2.Label {
            id: mergeText

            objectName: "mergeText"
            width: parent.width
            wrapMode: Text.Wrap
            text: (page.summary.created ? qsTr("Backup of %1.").arg(page.summary.created.toLocaleString(Qt.locale(), Locale.ShortFormat)) + "\n\n" : "")
                  + page.changes(page.summary)
        }
    }

    // The services read their data anew after a merge.
    Connections {
        target: Backup
        function onRestored() {
            Practice.refresh()
            Courses.refresh()
        }
    }
}
