import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Settings: the video bundles (codes of the DVDs), a folder with exam videos of one's own, and
// the backup (save everything as a file, read one back: it is merged, see Backup).
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
