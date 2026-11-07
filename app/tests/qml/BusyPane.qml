import org.kde.kirigami as Kirigami

// A page beside the list that is busy like a notebook recording (no microphone here): it asks to
// finish, and is done when the test says so.
Kirigami.Page {
    property bool busy: true
    property int finishAsked: 0

    function finishBusy() {
        finishAsked++
    }

    title: "Busy"
}
