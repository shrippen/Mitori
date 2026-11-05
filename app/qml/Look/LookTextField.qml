import QtQuick
import QtQuick.Controls as QQC2
import Kante

// LookTextField: KanteTextField under Breeze, Material, Kante and Leinen; under Shinki +shinki/LookTextField.qml.
// Pages use the part of its interface that ShinkiTextField has too.
KanteTextField {
    id: field

    // What to type, inside the field. Not placeholderText: Material floats that above the field
    // when it has the focus, cut off at the edge of a bar and across the frame.
    property string hint: ""

    QQC2.Label {
        objectName: "fieldHint"
        x: field.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        width: field.availableWidth
        visible: field.hint !== "" && field.text === "" && field.preeditText === ""
        text: field.hint
        elide: Text.ElideRight
        color: field.placeholderTextColor
        font: field.font
    }
}
