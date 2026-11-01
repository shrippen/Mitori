import QtQuick
import QtTest
import Mitori

// The pages as the user meets them: start menu, programme, technique sheet, comment, player.
// Screenshots go to uiShotsDir (for a look, not compared).
TestCase {
    id: test

    name: "Ui"
    when: windowShown

    property var window: null

    function initTestCase() {
        const component = Qt.createComponent("Mitori", "Main")
        verify(component.status === Component.Ready, component.errorString())
        window = component.createObject(null, { width: 420, height: 800 })
        window.show()
        tryVerify(() => window.pageStack.currentItem !== null)
    }

    function cleanupTestCase() {
        window.destroy()
    }

    function shot(name) {
        wait(800)       // Kirigami slides pages in
        let done = false
        window.pageStack.grabToImage(result => {
            result.saveToFile(uiShotsDir + "/" + name + ".png")
            done = true
        })
        tryVerify(() => done)
    }

    function push(page, properties) {
        window.pageStack.push(Qt.resolvedUrl("qrc:/qt/qml/Mitori/qml/" + page + ".qml"), properties || {})
        tryVerify(() => window.pageStack.currentItem && window.pageStack.currentItem.title !== undefined)
        return window.pageStack.currentItem
    }

    function test_1_start() {
        compare(Programme.errorString, "")
        compare(window.pageStack.currentItem.title, "Mitori 見取り")
        shot("1-start")
    }

    function test_2_programme() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const page = push("ProgrammePage")
        compare(page.grade, "k5")
        verify(page.rows.filter(r => r.kind === "entry").length === 20)
        shot("2-programme-aikido")

        Programme.discipline = "shinkitoho"
        page.grade = Programme.grade("shinkitoho")
        compare(page.grade, "shoden")
        verify(page.rows.some(r => r.kind === "info"))
        shot("3-programme-shinkitoho")
        window.pageStack.pop()
    }

    function test_2b_sheet() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const page = push("ProgrammePage")
        const entry = page.rows.find(r => r.kind === "entry" && r.attack)
        page.open(entry)
        const sheet = findChild(page, "techniqueSheet")
        verify(sheet)
        tryVerify(() => sheet.opened)
        compare(sheet.entry.id, entry.id)
        shot("2b-sheet")
        sheet.close()
        window.pageStack.pop()
    }

    function test_3_comment() {
        const entry = Programme.entry("shinkitoho/odachi/1")
        verify(entry.comment)
        const page = push("CommentPage", { entry: entry })
        compare(page.title, entry.comment.titel)
        shot("4-comment")
        window.pageStack.pop()
    }

    function test_3b_settings() {
        const page = push("SettingsPage")
        compare(page.title, "Settings")
        shot("4b-settings")
        window.pageStack.pop()
    }

    function test_4_player() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry" && r.video)
        if (rows.length === 0 || !Videos.available(rows[0].video.video)) {
            skip("no video for Aikido 5. Kyu (set MITORI_VIDEOS)")
        }
        const page = push("PlayerPage", { playlist: rows, index: 1 })
        tryVerify(() => page.playing, 10000)
        tryVerify(() => page.position >= page.from && page.position < page.to, 10000)
        wait(1500)
        shot("5-player")
        window.width = 900
        window.height = 420
        wait(500)
        shot("6-player-landscape")
        window.width = 420
        window.height = 800
        window.pageStack.pop()
    }

    // The last chapter ends with the file; a player at the end of the file no longer seeks back,
    // so the loop must turn before it (review of PR 3).
    function test_5_loop_turns_before_end_of_file() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry" && r.video)
        if (rows.length === 0 || !Videos.available(rows[0].video.video)) {
            skip("no video for Aikido 5. Kyu (set MITORI_VIDEOS)")
        }
        const page = push("PlayerPage", { playlist: rows, index: rows.length - 1 })
        tryVerify(() => page.duration > 0, 10000)
        verify(page.to >= page.duration - 50, "the last chapter should end with the file")
        verify(page.end < page.duration - 100)
        verify(page.end > page.to - 1000)
        window.pageStack.pop()
    }
}
