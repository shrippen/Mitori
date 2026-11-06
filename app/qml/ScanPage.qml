import QtQuick
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami
import Mitori

// The camera for the QR code of a DVD (services/codescanner): `done(code)` gets the code and the
// page closes. A QR code without a code gets a sentence and the camera keeps running.
//
//   [ camera picture       ]
//   Hold the QR code …
//   [Cancel]
Kirigami.Page {
    id: page

    // Called with the code (the page that opened the camera).
    property var done: null
    property bool foreignSeen: false

    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    title: qsTr("Scan code")
    padding: 0

    Component.onCompleted: CodeScanner.watch(output.videoSink)
    Component.onDestruction: CodeScanner.stop()

    Connections {
        target: CodeScanner

        function onFound(code) {
            if (page.done) {
                page.done(code)
            }
            page.Kirigami.PageStack.pop()
        }
        function onForeign() {
            page.foreignSeen = true
        }
    }

    CaptureSession {
        camera: Camera {
            id: camera

            // Off in the background: no picture is taken that nobody sees.
            active: CodeScanner.liveCamera && Qt.application.state !== Qt.ApplicationSuspended
                    && Qt.application.state !== Qt.ApplicationHidden
        }
        videoOutput: output
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.bottomMargin: Kirigami.Units.gridUnit

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Look.stageColor

            VideoOutput {
                id: output

                objectName: "scanOutput"
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
                Accessible.role: Accessible.Graphic
                Accessible.name: qsTr("Camera picture")
            }
        }

        LookLabel {
            objectName: "scanHint"
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            text: camera.errorString !== "" ? camera.errorString
                : page.foreignSeen ? qsTr("This QR code holds no DVD code. Use the one on the insert of the DVD.")
                : qsTr("Hold the QR code from the insert of the DVD in front of the camera.")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        LookButton {
            objectName: "scanCancel"
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Cancel")
            onClicked: page.Kirigami.PageStack.pop()
        }
    }
}
