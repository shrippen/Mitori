import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import Kante
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
        compare(Practice.level(entry.id), 3)
        compare(picker.value, 3)
        shot("2d-sheet-rated", true)
        // The grade's bar counts the new rating.
        const stand = findChild(page, "gradeStand")
        verify(stand)
        compare(stand.counts[3], 1)
        compare(stand.counts[0], stand.ids.length - 1)
        sheet.close()
        tryVerify(() => !sheet.opened)
        shot("2e-programme-rated")
        window.pageStack.pop()
    }

    // A row near the bottom of the window opens its sheet too (Kirigami keeps its notification
    // area in the overlay there; it must not swallow the taps).
    function test_2f_tap_row_at_bottom() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const page = push("ProgrammePage")
        wait(500)
        const sheet = findChild(page, "techniqueSheet")
        const list = findChild(page, "programmeList")
        // An entry row brought to the bottom edge of the list.
        const last = page.rows.map(r => r.kind).lastIndexOf("entry")
        list.positionViewAtIndex(last - 1, ListView.End)
        wait(300)
        let lowest = null, lowestY = -1
        for (const loader of list.contentItem.children) {
            const row = loader.item
            if (!row || !loader.visible || !loader.modelData || loader.modelData.kind !== "entry") {
                continue
            }
            const y = row.mapToItem(null, 0, 0).y
            if (y + row.height <= window.height && y > lowestY) {
                lowest = row
                lowestY = y
            }
        }
        verify(lowest && lowestY + lowest.height > window.height - 60, "an entry row at the bottom edge (" + lowestY + ")")
        mouseClick(lowest, lowest.width / 3, lowest.height / 2)
        tryVerify(() => sheet.opened, 2000, "a tap near the bottom opens the sheet")
        sheet.close()
        tryVerify(() => !sheet.opened)
        window.pageStack.pop()
    }

    // The visible level picker of an entry (in the overlay: sheets of earlier pages may linger).
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
        // Kante's ground, not the platform's (ScrollablePage does not inherit the theme).
        verify(Qt.colorEqual(page.background.color, KanteStyle.backgroundColor),
               page.background.color + " is not " + KanteStyle.backgroundColor)
        // and the scope reaches the page's controls: Kante's text colour, not the platform's.
        verify(Qt.colorEqual(page.Kirigami.Theme.textColor, KanteStyle.textColor),
               page.Kirigami.Theme.textColor + " is not " + KanteStyle.textColor)
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
        verify(Qt.colorEqual(page.background.color, KanteStyle.backgroundColor), "Kante's ground")
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

    // Today's practice: two new techniques of the current grade; rating one brings the next.
    function test_7_today() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const page = push("TodayPage")
        compare(page.discipline, "aikido")
        // Two new ones a day, less those of the grade already rated today (earlier cases).
        const newToday = Programme.entries("aikido", "k5").filter(r => r.kind === "entry" && Practice.ratedToday(r.id)).length
        verify(newToday < 2, "room for a new one today")
        compare(page.due.length, 2 - newToday)
        verify(page.due.every(r => r.level === 0 && r.current))
        const first = page.due[0].id
        verify(!Practice.ratedToday(first))
        Practice.rate(first, 2)
        // At most two new ones a day: the one practised counts, no other new one comes.
        compare(page.due.length, 1 - newToday)
        verify(page.due.every(r => r.id !== first), "practised today: not due")
        compare(findChild(page, "dueCount").value, String(1 - newToday))
        verify(Qt.colorEqual(page.background.color, KanteStyle.backgroundColor), "Kante's ground")
        shot("7-today")
        if (page.due.length > 0) {
            Practice.rate(page.due[0].id, 1)
        }
        compare(page.due.length, 0, "everything practised for today")
        window.pageStack.pop()
    }

    // Training plays today's due techniques; a rating moves on to the next open one.
    function test_7b_training() {
        // Daitoryu: nothing practised there yet today, two new ones are due.
        Programme.discipline = "daitoryu"
        Programme.setGrade("daitoryu", "k5")
        const today = push("TodayPage")
        const playable = today.due.filter(r => !!r.video && Videos.available(r.video.video))
        if (playable.length < 2) {
            skip("no video for Daitoryu 5. Kyu (set MITORI_VIDEOS)")
        }
        today.startTraining()
        tryVerify(() => window.pageStack.currentItem.training === true)
        const player = window.pageStack.currentItem
        compare(player.index, 0)
        // Skip the first with "next", rate the second: the training goes back to the skipped one.
        player.go(1)
        compare(player.index, 1)
        findPicker(player, player.current.id).picked(3)
        compare(player.index, 0, "back to the open one that was skipped")
        findPicker(player, player.current.id).picked(2)
        if (player.playlist.length > 2) {
            compare(player.index, 2, "rating moves on")
        }
        // The last one (and every one before it) practised: done, with a way back.
        for (let i = 1; i < player.playlist.length; ++i) {
            player.index = i
            if (!Practice.ratedToday(player.current.id)) {
                Practice.rate(player.current.id, 2)
            }
        }
        verify(player.trainingDone)
        verify(findChild(player, "trainingDone").visible)
        window.pageStack.pop()
        tryVerify(() => window.pageStack.currentItem === today)
        window.pageStack.pop()
    }

    // Notes: add one for today from the field, change it and delete it (press and hold).
    function test_8_notes() {
        const entry = Programme.entries("aikido", "k5").find(r => r.kind === "entry")
        const page = push("NotesPage", { entry: entry })
        compare(page.notes.length, 0)
        const compose = findChild(page, "compose")
        compose.text = "Hüfte früher drehen"
        compose.accepted()                              // Enter in the field
        compare(page.notes.length, 1)
        compare(compose.text, "")
        compare(Practice.lastNote(entry.id).text, "Hüfte früher drehen")
        compare(page.days.length, 1)
        Practice.addNote(entry.id, "Blick beim Partner")
        compare(page.days[0].notes.length, 2)
        shot("8-notes")
        const note = findChild(page, "note")
        verify(note)
        mousePress(note, 20, note.height / 2)
        wait(1000)
        mouseRelease(note, 20, note.height / 2)
        const editor = findChild(page, "noteEditor")
        tryVerify(() => editor.opened, 2000, "press and hold opens the editor")
        compare(editor.note.uid, page.notes[0].uid)
        // Save from the editor.
        findChild(editor, "noteArea").text = "Hüfte viel früher drehen"
        editor.customFooterActions[1].trigger()
        tryVerify(() => !editor.opened)
        compare(page.notes[0].text, "Hüfte viel früher drehen")
        // Right-click the second note, delete it from the editor.
        wait(500)                                       // the editor's overlay fades out
        const second = findChildren(page, "note").find(n => n.modelData.uid === page.notes[1].uid)
        mouseClick(second, 20, second.height / 2, Qt.RightButton)
        tryVerify(() => editor.opened)
        compare(editor.note.uid, page.notes[1].uid)
        editor.customFooterActions[0].trigger()
        tryVerify(() => !editor.opened)
        compare(page.notes.length, 1)
        compare(findChild(page, "dayName").text, "Today")
        window.pageStack.pop()
    }

    function findChildren(item, name) {
        let found = []
        if (item.objectName === name) {
            found.push(item)
        }
        for (let i = 0; i < item.children.length; i++) {
            found = found.concat(findChildren(item.children[i], name))
        }
        return found
    }

    // A note begun in the player belongs to the technique it was typed for, also when the player
    // moves on before it is added (training moves on after a rating).
    function test_8b_note_draft_stays_with_its_technique() {
        const page = playerAt(1)
        const first = page.current.id
        const field = findChild(page, "noteField")
        field.text = "für die erste Technik"
        page.go(1)
        verify(page.current.id !== first)
        compare(Practice.lastNote(first).text, "für die erste Technik", "saved for the technique it was typed for")
        verify(Practice.lastNote(page.current.id).text !== "für die erste Technik")
        compare(field.text, "", "the field is empty for the next technique")
        // Leaving the player keeps a draft as well.
        const second = page.current.id
        field.text = "beim Verlassen"
        window.pageStack.pop()
        tryVerify(() => Practice.lastNote(second).text === "beim Verlassen", 3000, "saved when the player closes")
    }

    // With the note field focused, the controls scroll it into view when the window shrinks
    // (the on-screen keyboard opens after the focus).
    function test_8c_note_field_above_keyboard() {
        window.width = 800
        window.height = 360
        const page = playerAt(0)
        const view = findChild(page, "controlsView")
        const field = findChild(page, "noteField")
        field.forceActiveFocus()
        window.height = 200
        wait(300)
        const at = field.mapToItem(view, 0, 0)
        verify(at.y >= 0 && at.y + field.height <= view.height + 1, "field in view (" + at.y + ", " + view.height + ")")
        window.width = 420
        window.height = 800
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
