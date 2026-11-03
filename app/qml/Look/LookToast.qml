import QtQuick
import Kante

// A toast: `importance` Info or Warning, title, text, show(), closed(). Kante: KanteToast. Under
// Shinki +shinki/LookToast.qml (ShinkiToast).
KanteToast {
    enum Importance {
        Info,
        Warning
    }
    property int importance: LookToast.Importance.Info

    kind: importance === LookToast.Importance.Warning ? KanteToast.Kind.Warn : KanteToast.Kind.Info
}
