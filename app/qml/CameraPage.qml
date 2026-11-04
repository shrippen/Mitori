import QtQuick
import QtQuick.Controls as QQC2
import QtMultimedia
import org.kde.kirigami as Kirigami
import Mitori

// The camera for a note's photo: the picture fills the page, a button takes it; the photo goes
// into the media folder (Media.importCapture, made smaller), `done(name)` gets it and the page
// closes.
Kirigami.Page {
    id: page

    // Called with the photo's name (the page that opened the camera).
    property var done: null
    property string captureError: ""

    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.stageColor }

    title: qsTr("Photo")
    padding: 0

    MediaDevices {
        id: devices
    }

    CaptureSession {
        camera: Camera {
            id: camera

            active: true
        }
        imageCapture: ImageCapture {
            id: capture

            onImageSaved: (id, path) => {
                const name = Media.importCapture(path)
                if (name !== "" && page.done) {
                    page.done(name)
                }
                page.Kirigami.PageStack.pop()
            }
            onErrorOccurred: (id, error, message) => page.captureError = message
        }
        videoOutput: output
    }

    VideoOutput {
        id: output

        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }

    LookLabel {
        id: failed

        objectName: "cameraError"
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 2
        visible: text !== ""
        text: page.captureError !== "" ? page.captureError
              : camera.errorString !== "" ? camera.errorString
              : devices.videoInputs.length === 0 ? qsTr("No camera found.") : ""
        color: Look.stageTextColor
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }

    LookButton {
        objectName: "shutter"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Kirigami.Units.gridUnit
        role: LookButton.Role.Primary
        enabled: capture.readyForCapture
        icon.name: "camera-photo"
        text: qsTr("Take photo")
        onClicked: capture.captureToFile(Media.cameraFile())
    }
}
