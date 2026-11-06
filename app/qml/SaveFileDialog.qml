import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import Mitori

// Saving a ZIP to a file the user picks: the platform's dialog, a typed path taken as meant, and a
// question before an existing file is replaced. `chosen` gives the target.
FileDialog {
    id: root

    // The page: the question lies on its window's overlay.
    required property Item anchor

    signal chosen(url target)

    // Opens with this name in the folder last used.
    function ask(name) {
        root.selectedFile = root.currentFolder + "/" + encodeURIComponent(name)
        root.open()
    }

    // The dialog's answer; tests start here (the file dialog is the platform's).
    function take(picked) {
        const target = SavePlace.resolve(picked, root.currentFolder)
        if (!SavePlace.taken(target)) {
            root.chosen(target)
            return
        }
        replaceDialog.target = target
        replaceDialog.open()
    }

    fileMode: FileDialog.SaveFile
    defaultSuffix: "zip"
    nameFilters: [qsTr("ZIP archives (*.zip)")]
    currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
    options: SavePlace.asksItself ? FileDialog.DontConfirmOverwrite : 0
    onAccepted: root.take(root.selectedFile)

    LookDialog {
        id: replaceDialog

        property url target

        objectName: "replaceFile"
        parent: root.anchor.QQC2.Overlay.overlay
        anchors.centerIn: parent
        modal: true
        destructive: true
        span: LookDialog.Span.Small
        title: qsTr("Replace file?")
        acceptText: qsTr("Replace")
        standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
        onAccepted: root.chosen(replaceDialog.target)

        LookLabel {
            width: parent.width
            text: qsTr("%1 is there already.").arg(decodeURIComponent(replaceDialog.target.toString().replace(/^file:\/\//, "")))
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }
    }
}
