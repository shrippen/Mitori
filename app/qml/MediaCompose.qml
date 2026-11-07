import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami
import Mitori

// Photo and voice note beside a note field (clickdummy: 📷 🎤): a photo from the camera or a
// picture file, or a voice note recorded here. Each gives `attached(name)` (Media); the page
// saves the note with the field's text. Camera and microphone ask for permission first; a
// refused permission or an unreadable picture says so in a toast.
//
//   [📷] [🎤]                   idle
//   ● 0:12  [Stop] [Cancel]     recording
Item {
    id: compose

    // A photo or voice note is ready: its name in the media folder.
    signal attached(string name)
    readonly property bool recording: recorder.recorderState === MediaRecorder.RecordingState
    // The name the recorder writes to (Media.newVoice), until it is kept or discarded.
    property string voiceName: ""
    // The camera page goes on this page's stack.
    property Item stackPage: null

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    RowLayout {
        id: row

        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        LookToolButton {
            objectName: "addPhoto"
            visible: !compose.recording
            icon.name: "camera-photo"
            text: qsTr("Photo")
            display: QQC2.AbstractButton.IconOnly
            onClicked: photoMenu.open()

            LookMenu {
                id: photoMenu

                LookMenuItem {
                    objectName: "takePhoto"
                    icon.name: "camera-photo"
                    text: qsTr("Take a photo")
                    onTriggered: compose.takePhoto()
                }
                LookMenuItem {
                    objectName: "choosePicture"
                    icon.name: "insert-image"
                    text: qsTr("Choose a picture …")
                    onTriggered: pictureDialog.open()
                }
            }
        }
        LookToolButton {
            objectName: "recordVoice"
            visible: !compose.recording
            icon.name: "audio-input-microphone"
            text: qsTr("Voice note")
            display: QQC2.AbstractButton.IconOnly
            onClicked: compose.record()
        }

        // While recording: time, Stop (keeps it), Cancel.
        LookLabel {
            objectName: "recordingTime"
            visible: compose.recording
            text: "● " + Math.floor(recorder.duration / 60000) + ":" + String(Math.floor(recorder.duration / 1000) % 60).padStart(2, "0")
            color: Look.negativeTextColor
            Accessible.name: qsTr("Recording")
        }
        LookButton {
            objectName: "stopRecording"
            visible: compose.recording
            icon.name: "media-playback-stop"
            text: qsTr("Stop")
            onClicked: recorder.stop()
        }
        LookButton {
            objectName: "cancelRecording"
            visible: compose.recording
            text: qsTr("Cancel")
            onClicked: {
                recorder.cancelled = true
                recorder.stop()
            }
        }

    }

    function takePhoto() {
        if (cameraPermission.status !== Qt.PermissionStatus.Granted) {
            if (cameraPermission.status === Qt.PermissionStatus.Denied && compose.cameraAsked) {
                compose.tell(qsTr("Mitori may not use the camera. Allow it in the system settings."))
            }
            compose.cameraAsked = true
            cameraPermission.request()
            return
        }
        compose.stackPage.Kirigami.PageStack.push(Qt.resolvedUrl("CameraPage.qml"), { done: name => compose.attached(name) })
    }

    // Stop as with the button: the note is kept (`attached`).
    function finish() {
        if (recording) {
            recorder.stop()
        }
    }

    // The page goes in mid-recording: nothing to give the note to, no file stays.
    Component.onDestruction: {
        if (voiceName === "") {
            return
        }
        const name = voiceName
        voiceName = ""
        recorder.stop()
        Media.discard(name)
    }

    function record() {
        if (microphonePermission.status !== Qt.PermissionStatus.Granted) {
            if (microphonePermission.status === Qt.PermissionStatus.Denied && compose.microphoneAsked) {
                compose.tell(qsTr("Mitori may not use the microphone. Allow it in the system settings."))
            }
            compose.microphoneAsked = true
            microphonePermission.request()
            return
        }
        const voice = Media.newVoice()
        compose.voiceName = voice.name
        recorder.cancelled = false
        recorder.outputLocation = voice.url
        recorder.record()
    }

    // A picked file: in as a photo, or a message (not a picture Qt can read).
    function importPicture(url) {
        const name = Media.importPhoto(url)
        if (name === "") {
            compose.tell(qsTr("This file could not be read as a picture."))
            return
        }
        compose.attached(name)
    }

    function tell(text) {
        problem.text = text
        problem.show()
    }

    // On the window's overlay, bottom right.
    property LookToast problem: LookToast {
        objectName: "mediaProblem"
        parent: QQC2.Overlay.overlay
        anchors.right: parent ? parent.right : undefined
        anchors.bottom: parent ? parent.bottom : undefined
        anchors.margins: Kirigami.Units.largeSpacing
        importance: LookToast.Importance.Warning
    }
    // Asked once already this time: a refusal now is a lasting one (Android asks no more).
    property bool cameraAsked: false
    property bool microphoneAsked: false


    CameraPermission {
        id: cameraPermission

        onStatusChanged: {
            if (status === Qt.PermissionStatus.Granted) {
                compose.takePhoto()
            } else if (status === Qt.PermissionStatus.Denied) {
                compose.tell(qsTr("Mitori may not use the camera. Allow it in the system settings."))
            }
        }
    }
    MicrophonePermission {
        id: microphonePermission

        onStatusChanged: {
            if (status === Qt.PermissionStatus.Granted) {
                compose.record()
            } else if (status === Qt.PermissionStatus.Denied) {
                compose.tell(qsTr("Mitori may not use the microphone. Allow it in the system settings."))
            }
        }
    }

    FileDialog {
        id: pictureDialog

        title: qsTr("Choose a picture")
        nameFilters: [qsTr("Pictures (*.jpg *.jpeg *.png *.webp)")]
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        onAccepted: compose.importPicture(selectedFile)
    }

    // AAC in MP4: small, and played by every platform's player.
    CaptureSession {
        audioInput: AudioInput {}
        recorder: MediaRecorder {
            id: recorder

            property bool cancelled: false

            mediaFormat {
                fileFormat: MediaFormat.MPEG4
                audioCodec: MediaFormat.AudioCodec.AAC
            }
            quality: MediaRecorder.NormalQuality
            onRecorderStateChanged: {
                if (recorderState !== MediaRecorder.StoppedState || compose.voiceName === "") {
                    return
                }
                const name = compose.voiceName
                compose.voiceName = ""
                if (cancelled || duration < 500) {          // a tap on Stop right away is no note
                    Media.discard(name)
                    return
                }
                compose.attached(name)
            }
        }
    }
}
