import QtQuick
import Kante

// A hint card: an importance (info, important, error), a title, a text and actions as children.
// Kante: KanteHintCard. Under Shinki +shinki/LookHint.qml (ShinkiHint).
KanteHintCard {
    enum Importance {
        Info,
        Important,
        Error
    }
    property int importance: LookHint.Importance.Info

    tier: importance === LookHint.Importance.Error ? KanteHintCard.Tier.Red
        : importance === LookHint.Importance.Important ? KanteHintCard.Tier.Primary : KanteHintCard.Tier.Blue
}
