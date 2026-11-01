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

    // The page stack, or with `whole` the window with its overlay (dialogs, sheets).
    function shot(name, whole) {
        wait(800)       // Kirigami slides pages in
        let done = false
        const item = whole ? window.overlay.parent : window.pageStack
        item.grabToImage(result => {
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
        shot("2b-sheet", true)
        sheet.close()
        window.pageStack.pop()
    }

    // Rating in the sheet: the list row shows the level as dots, the sheet the last rating.
    function test_2d_rate_in_sheet() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const page = push("ProgrammePage")
        const entry = page.rows.find(r => r.kind === "entry")
        compare(Practice.level(entry.id), 0)
        page.open(entry)
        const sheet = findChild(page, "techniqueSheet")
        tryVerify(() => sheet.opened)
        const picker = findPicker(window.overlay, entry.id)
        verify(picker, "the sheet has the level picker for its entry")
        // The sheet shows its content, not only its title: the picker lies inside it, visibly.
        wait(300)
        verify(picker.height > 0 && picker.width > 0, "picker has a size")
        const at = picker.mapToItem(null, 0, 0)
        const box = sheet.background.mapToItem(null, 0, 0)
        verify(at.y >= box.y && at.y + picker.height <= box.y + sheet.background.height,
               "picker within the sheet (" + at.y + " + " + picker.height + " outside " + box.y + " + " + sheet.background.height + ")")
        wait(500)       // the sheet slides in
        mouseClick(picker, picker.width * 5 / 8, picker.height / 2)     // the third of four buttons
        tryCompare(Practice, "revision", 1)
        console.log("dbg rated", page.rows.filter(r => Practice.level(r.id) > 0).map(r => r.id + "=" + Practice.level(r.id)).join(","), picker.parent.entry)
        compare(Practice.level(entry.id), 3)
        compare(picker.value, 3)
        shot("2d-sheet-rated", true)
        sheet.close()
        tryVerify(() => !sheet.opened)
        shot("2e-programme-rated")
        window.pageStack.pop()
    }

    // The visible level picker of an entry (in the overlay: sheets of earlier pages may linger).
    // A row near the bottom of the window opens its sheet too (Kirigami keeps its notification
    // area in the overlay there; it must not swallow the taps).
    function test_2f_tap_row_at_bottom() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const page = push("ProgrammePage")
        wait(500)
        const sheet = findChild(page, "techniqueSheet")
        const y = window.height - 60
        mouseClick(window.contentItem, window.width / 3, y - window.contentItem.mapToItem(null, 0, 0).y)
        tryVerify(() => sheet.opened, 2000, "a tap near the bottom opens the sheet")
        sheet.close()
        tryVerify(() => !sheet.opened)
        window.pageStack.pop()
    }

    function findPicker(item, id) {
        if (!item) {
            return null
        }
        if (item.picked !== undefined && item.focusLevel !== undefined && item.parent.entry === id && item.visible) {
            return item
        }
        for (let i = 0; i < item.children.length; i++) {
            const found = findPicker(item.children[i], id)
            if (found) {
                return found
            }
        }
        return null
    }

    // Entering the video folder in the settings: the list behind them shows the videos at once.
    function test_2c_folder_change() {
        const real = Videos.folder
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry" && r.video)
        if (rows.length === 0 || !Videos.available(rows[0].video.video)) {
            skip("no video for Aikido 5. Kyu (set MITORI_VIDEOS)")
        }
        Videos.folder = uiShotsDir
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const page = push("ProgrammePage")
        verify(!page.hasVideo)
        Videos.folder = real
        verify(page.hasVideo, "the list follows the folder")
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
        // A phone held sideways: the controls scroll, and Back can be reached.
        window.width = 800
        window.height = 360
        wait(500)
        const view = findChild(page, "controlsView")
        const back = findChild(page, "backButton")
        verify(view && back)
        verify(back.visible)
        view.contentY = Math.max(0, view.contentHeight - view.height)
        wait(100)
        const at = back.mapToItem(view, 0, 0)
        verify(at.y >= 0 && at.y + back.height <= view.height + 1, "Back in view after scrolling (" + at.y + ", " + view.height + ")")
        window.width = 420
        window.height = 800
        window.pageStack.pop()
    }

    function playerAt(index) {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry" && r.video)
        if (rows.length === 0 || !Videos.available(rows[0].video.video)) {
            skip("no video for Aikido 5. Kyu (set MITORI_VIDEOS)")
        }
        const page = push("PlayerPage", { playlist: rows, index: index < 0 ? rows.length + index : index })
        tryVerify(() => page.playing && page.position > page.from + 500, 10000)
        return page
    }

    // Steps and jumps stay where they land (the backend reports "loaded" after every seek).
    function test_5_step_and_jump_stay() {
        const page = playerAt(0)
        page.togglePlay()
        tryVerify(() => !page.playing)
        const before = page.position
        page.step(1)
        tryVerify(() => page.position >= before + page.frameMs - 5, 3000)
        wait(500)
        verify(!page.playing)
        verify(page.position > page.from + 400)
        page.jump(3000)
        tryVerify(() => page.position > before + 2500, 3000)
        wait(500)
        verify(page.position > before + 2500)
        window.pageStack.pop()
    }

    // Next technique in the same video: plays the new chapter, not the old one from its start.
    function test_5b_next_technique() {
        const page = playerAt(0)
        const firstTo = page.to
        page.go(1)
        tryVerify(() => page.from === firstTo, 2000)
        tryVerify(() => page.position >= page.from && page.position < page.to, 5000)
        wait(800)
        verify(page.position >= page.from, "still in the new chapter")
        window.pageStack.pop()
    }

    // ▶ in the list opens the player; the list marks the entry the player showed last.
    function test_5c_list_marks_last_shown() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const list = push("ProgrammePage")
        if (!list.hasVideo) {
            skip("no video for Aikido 5. Kyu (set MITORI_VIDEOS)")
        }
        const playable = list.rows.filter(r => list.playable(r))
        list.play(playable[1])
        tryVerify(() => window.pageStack.currentItem !== list && window.pageStack.currentItem.playlist !== undefined)
        const player = window.pageStack.currentItem
        compare(list.lastId, playable[1].id)
        player.go(1)
        compare(list.lastId, playable[2].id)
        window.pageStack.pop()
        tryVerify(() => window.pageStack.currentItem === list)
        window.pageStack.pop()
    }

    // Next technique in another video (Shoden: the suburi, then the forms): its own chapter too.
    function test_5d_next_technique_other_file() {
        const rows = Programme.entries("shinkitoho", "shoden").filter(r => r.kind === "entry" && r.video)
        const at = rows.findIndex((r, i) => i > 0 && r.video.video !== rows[i - 1].video.video)
        verify(at > 0)
        if (!Videos.available(rows[at - 1].video.video) || !Videos.available(rows[at].video.video)) {
            skip("no video for Shinki Toho Shoden (set MITORI_VIDEOS)")
        }
        const page = push("PlayerPage", { playlist: rows, index: at - 1 })
        tryVerify(() => page.playing && page.position > page.from + 500, 10000)
        page.go(1)
        compare(page.video, rows[at].video.video)
        tryVerify(() => page.playing && page.position >= page.from && page.position < page.to, 10000)
        wait(800)
        verify(page.position >= page.from && page.position < page.to, "in the chapter of the new file")
        window.pageStack.pop()
    }

    // The transport bar's keys work right after opening; Space and the arrows stay single.
    function test_5e_keys_without_click() {
        const page = playerAt(0)
        page.forceActiveFocus()
        keyClick(Qt.Key_K)
        tryVerify(() => !page.playing)
        keyClick(Qt.Key_Minus)
        compare(page.rate, 0.75)
        keyClick(Qt.Key_Space)
        tryVerify(() => page.playing)
        wait(300)
        verify(page.playing, "Space toggles once")
        keyClick(Qt.Key_K)
        tryVerify(() => !page.playing)
        const before = page.position
        keyClick(Qt.Key_Right)
        tryVerify(() => page.position >= before + page.frameMs - 5, 3000)
        window.pageStack.pop()
    }

    // The last chapter ends with the file; the loop must still start over at the chapter.
    function test_6_loop_at_end_of_file() {
        const page = playerAt(-1)
        // The turning point keeps clear of the end of the file, where the player stops for good.
        verify(page.end <= page.duration - 250)
        page.jump(page.to - page.position - 3000)
        tryVerify(() => page.position > page.to - 2500, 5000)
        tryVerify(() => page.position < page.from + 3000, 8000)
        verify(page.playing)
        window.pageStack.pop()
    }
}
