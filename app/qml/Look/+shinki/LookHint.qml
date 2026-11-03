import Shinki

ShinkiHint {
    enum Importance {
        Info,
        Important,
        Error
    }
    property int importance: LookHint.Importance.Info

    kind: importance === LookHint.Importance.Error ? ShinkiHint.Kind.Error
        : importance === LookHint.Importance.Important ? ShinkiHint.Kind.Important : ShinkiHint.Kind.Info
}
