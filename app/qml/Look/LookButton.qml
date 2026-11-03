import QtQuick
import Kante

// A button; `role` Primary for the one main action of a view. Kante: KanteButton. Under Shinki
// +shinki/LookButton.qml (ShinkiButton).
KanteButton {
    enum Role {
        Normal,
        Primary,
        Quiet
    }
    property int role: LookButton.Role.Normal

    emphasis: role === LookButton.Role.Primary ? KanteButton.Emphasis.Primary
        : role === LookButton.Role.Quiet ? KanteButton.Emphasis.Quiet : KanteButton.Emphasis.Normal
}
