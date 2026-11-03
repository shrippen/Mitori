import Shinki

ShinkiButton {
    enum Role {
        Normal,
        Primary
    }
    property int role: LookButton.Role.Normal

    primary: role === LookButton.Role.Primary
}
