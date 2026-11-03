import Shinki

ShinkiButton {
    enum Role {
        Normal,
        Primary,
        Quiet
    }
    property int role: LookButton.Role.Normal

    primary: role === LookButton.Role.Primary
    link: role === LookButton.Role.Quiet       // a way on ("All years …"): red and bold, no frame
}
