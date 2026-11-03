import Shinki

ShinkiToast {
    enum Importance {
        Info,
        Warning
    }
    property int importance: LookToast.Importance.Info

    kind: importance === LookToast.Importance.Warning ? ShinkiToast.Kind.Warn : ShinkiToast.Kind.Info
}
