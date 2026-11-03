import QtQuick
import QtMultimedia
import QtTest
import QtQuick.Controls as QQC2
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

    // A failed or skipped case (no videos, as in the CI) leaves no tablet window and no pages
    // to the next ones: each starts on the start page.
    function cleanup() {
        window.width = 420
        window.height = 800
        while (window.pageStack.depth > 1)
            window.pageStack.pop()
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

    // A start tile into view (the column of tiles is taller than a phone), then a click on it.
    function clickTile(tile) {
        const f = window.pageStack.items[0].flickable
        const y = tile.mapToItem(f.contentItem, 0, 0).y
        f.contentY = Math.max(0, Math.min(y - 20, f.contentHeight - f.height))
        wait(50)
        mouseClick(tile)
    }

    // WCAG contrast of two colours (text over ground, alpha laid on the ground).
    function contrast(fore, back) {
        const lin = c => c <= 0.03928 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4)
        const lum = c => 0.2126 * lin(c.r) + 0.7152 * lin(c.g) + 0.0722 * lin(c.b)
        const laid = Qt.rgba(fore.r * fore.a + back.r * (1 - fore.a), fore.g * fore.a + back.g * (1 - fore.a),
                             fore.b * fore.a + back.b * (1 - fore.a), 1)
        const a = lum(laid), b = lum(back)
        return (Math.max(a, b) + 0.05) / (Math.min(a, b) + 0.05)
    }

    function push(page, properties) {
        window.pageStack.push(Qt.resolvedUrl("qrc:/qt/qml/Mitori/qml/" + page + ".qml"), properties || {})
        tryVerify(() => window.pageStack.currentItem && window.pageStack.currentItem.title !== undefined)
        return window.pageStack.currentItem
    }

    // The first-start wizard: welcome, style, tour; the style waits for the next start; Start
    // marks it done and goes back to the start menu.
    function test_0_welcome() {
        const page = push("WelcomePage")
        const steps = findChild(page, "welcomeSteps")
        const next = findChild(page, "welcomeNext")
        compare(steps.currentIndex, 0)
        // Screen readers hear the steps' states in the app's language, not Kante's German default.
        compare(findChild(page, "welcomeStepper").stateNames.current, "current")
        shot("0-welcome")
        next.clicked()
        compare(steps.currentIndex, 1)
        const tiles = findChildren(page, "styleTile")
        verify(tiles.length >= 3, "Material, Kante, Leinen at least")
        const kante = tiles.find(t => t.text === "Kante")
        verify(kante)
        kante.clicked()
        compare(Appearance.style, Appearance.Style.Kante)
        verify(Appearance.restartNeeded)
        shot("0-welcome-style")
        next.clicked()
        compare(steps.currentIndex, 2)
        compare(findChildren(page, "tourRow").length, 5)
        shot("0-welcome-tour")
        next.clicked()
        tryVerify(() => window.pageStack.depth === 1)
        verify(Appearance.welcomed)
        Appearance.setStyle(Appearance.running)
        verify(!Appearance.restartNeeded)
    }

    // Settings → Appearance: the style tiles, the language, the restart hint, about Mitori.
    function test_0b_appearance() {
        const settings = push("SettingsPage")
        findChild(settings, "appearanceButton").clicked()
        tryVerify(() => window.pageStack.currentItem.title === "Appearance")
        const page = window.pageStack.currentItem
        const hint = findChild(page, "restartHint")
        verify(!hint.visible)
        const language = findChild(page, "languageChoice")
        compare(language.currentIndex, Appearance.Language.System)
        language.activated(Appearance.Language.German)
        compare(Appearance.language, Appearance.Language.German)
        verify(hint.visible)
        verify(findChild(page, "aboutText").text.indexOf(Appearance.version) >= 0)
        verify(Appearance.version !== "")
        shot("0b-appearance")
        language.activated(Appearance.Language.System)
        verify(!hint.visible)
    }

    function test_1_start() {
        compare(Programme.errorString, "")
        // The run's style reached Look (the file selector "shinki" in ui_shinki).
        compare(Look.shinki, uiShinki)
        compare(window.pageStack.currentItem.title, "Mitori 見取り")
        // The tiles in the decided order: Today, Exam, Statistics, Courses (clickdummy "Startmenü").
        const start = window.pageStack.currentItem
        const order = ["todayTile", "examTile", "statsTile", "coursesTile", "mapTile", "glossaryTile"]
        const items = order.map(n => findChild(start, n))
        const at = it => it.mapToItem(start, 0, 0)
        for (let i = 1; i < items.length; i++) {
            const a = at(items[i - 1]), b = at(items[i])
            verify(b.y > a.y || (b.y === a.y && b.x > a.x), order[i - 1] + " before " + order[i])
        }
        shot("1-start")
    }

    // Under Shinki the symbols on red and on ink are white (bible section 5), on every platform:
    // the play button and a checked tool button (loop, sound).
    function test_1c_icons_on_red() {
        if (!uiShinki) {
            skip("Shinki only")
        }
        const play = createTemporaryQmlObject("import Mitori; LookPlayButton {}", window.contentItem)
        const loop = createTemporaryQmlObject(
            "import QtQuick.Controls; import Mitori; LookToolButton { x: 100; checkable: true; checked: true; "
            + "icon.name: 'media-playlist-repeat'; display: AbstractButton.IconOnly }", window.contentItem)
        for (const button of [play, loop]) {
            tryVerify(() => button.width > 0)
            wait(100)
            const image = grabImage(button)
            // The brightest pixel inside the round ground (the corners show the page): white.
            let lightest = 0
            const cx = image.width / 2, cy = image.height / 2, r = Math.min(cx, cy) - 3
            for (let x = 0; x < image.width; x++) {
                for (let y = 0; y < image.height; y++) {
                    const c = image.pixel(x, y)
                    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) < r * r) {
                        lightest = Math.max(lightest, Math.min(c.r, c.g, c.b))
                    }
                }
            }
            verify(lightest > 0.9, button + ": the symbol is not white, lightest " + lightest)
        }
    }

    // A dialog of the content's size (Fit): its content measures the text.
    function test_1d_dialog_fit() {
        const dialog = createTemporaryQmlObject(
            "import QtQuick.Controls; import Mitori; LookDialog { Label { text: 'Hello' } }", window.contentItem)
        verify(dialog.contentWidth > 0, "content width " + dialog.contentWidth)
        tryVerify(() => dialog.contentHeight > 0, 1000, "content height " + dialog.contentHeight)  // Kante: once built
    }

    // A date field says when its text is no date (pages lock their button on it), and its month
    // opens inside the window, also at the window's bottom.
    function test_1e_date_field() {
        const field = createTemporaryQmlObject(
            "import Mitori; LookDateField { format: 'yyyy-MM-dd'; date: new Date(2026, 0, 5) }", window.contentItem)
        field.forceActiveFocus()
        keySequence("Ctrl+A")
        keyClick(Qt.Key_Delete)
        for (const c of "2026-02-31") {
            keyClick(c)
        }
        keyClick(Qt.Key_Return)
        compare(field.invalid, true, "31 February is no date")

        // Opened once at the top, then the page scrolls it to the bottom (as on a phone).
        const popup = findChild(field, "popup")
        field.open()
        tryVerify(() => popup.opened)
        field.close()
        tryVerify(() => !popup.visible)
        field.y = window.height - field.height - 10
        field.open()
        tryVerify(() => popup.opened)
        const top = popup.contentItem.mapToItem(null, 0, 0).y - popup.topPadding
        verify(top >= 0 && top + popup.height <= window.height, "month inside the window: " + top + " + " + popup.height)
        field.close()
        tryVerify(() => !popup.visible)

        // A month of six week rows after one of five (January, then March 2026), at the bottom:
        // still inside the window and off the field.
        field.date = new Date(2026, 0, 5)
        field.y = 0
        field.open()
        tryVerify(() => popup.opened)
        field.close()
        tryVerify(() => !popup.visible)
        field.date = new Date(2026, 2, 30)
        field.y = 392
        field.open()
        tryVerify(() => popup.opened)
        wait(50)
        const six = popup.contentItem.mapToItem(null, 0, 0).y - popup.topPadding
        const fieldTop = field.mapToItem(null, 0, 0).y
        verify(six >= 0 && six + popup.height <= window.height, "six rows inside the window: " + six + " + " + popup.height)
        verify(six + popup.height <= fieldTop || six >= fieldTop + field.height, "the month leaves the field free")
        field.close()
    }

    function test_2_programme() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const page = push("ProgrammePage")
        compare(page.grade, "k5")
        verify(page.rows.filter(r => r.kind === "entry").length === 20)
        // The list scrolls under the grade chips, not over them.
        verify(findChild(page, "programmeScroll").clip, "the list is clipped")
        shot("2-programme-aikido")

        // A tap on the chosen grade keeps it chosen (the chip does not toggle off).
        const chips = findChild(page, "gradeChips")
        const k5 = chips.itemAt(page.grades.findIndex(g => g.id === "k5"))
        mouseClick(k5)
        compare(page.grade, "k5")
        verify(k5.checked, "the chosen grade stays checked")
        for (let i = 0; i < chips.count; i++) {
            compare(chips.itemAt(i).checked, chips.itemAt(i) === k5, chips.itemAt(i).text)
        }

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
        // The head: attack, addition and grade above the name (clickdummy "Blatt").
        const eyebrow = findChild(sheet.contentItem, "sheetEyebrow")
        verify(eyebrow.text.indexOf(entry.attack) >= 0 && eyebrow.text.indexOf("5. Kyu") >= 0, eyebrow.text)
        // Without the video on the device: what to do (the grade has a video: enter the code).
        const hint = findChild(sheet.contentItem, "sheetNoVideo")
        compare(hint.visible, !sheet.hasVideo)
        if (!sheet.hasVideo) {
            verify(findChild(sheet.contentItem, "sheetCodeButton").visible)
        }
        // The notes button counts them, the last note has its date.
        Practice.addNote(entry.id, "Schritt kleiner")
        const notes = findChild(sheet.contentItem, "sheetNotesButton")
        verify(notes.text.indexOf("(1)") >= 0, notes.text)
        const lastNote = findChild(sheet.contentItem, "sheetLastNote")
        verify(lastNote.text.indexOf(new Date().toLocaleDateString(Qt.locale(), Locale.ShortFormat)) >= 0, lastNote.text)
        shot("2b-sheet", true)
        for (const n of Practice.notes(entry.id)) {
            Practice.deleteNote(n.uid)
        }
        sheet.close()
        window.pageStack.pop()
    }

    // A technique without any video (Shinki Toho Jun-Menkyo, the last grade with forms): the
    // sheet says so, no code to enter.
    function test_2b_sheet_no_video() {
        Programme.discipline = "shinkitoho"
        const grades = Programme.grades("shinkitoho")
        const last = grades[grades.length - 1]
        const before = Programme.grade("shinkitoho")
        Programme.setGrade("shinkitoho", last.id)
        const page = push("ProgrammePage")
        const entry = page.rows.find(r => r.kind === "entry" && !r.video)
        verify(entry, "an entry without a video in " + last.name)
        page.open(entry)
        const sheet = findChild(page, "techniqueSheet")
        tryVerify(() => sheet.opened)
        verify(findChild(sheet.contentItem, "sheetNoVideo").visible)
        verify(!findChild(sheet.contentItem, "sheetCodeButton").visible)
        sheet.close()
        window.pageStack.pop()
        Programme.setGrade("shinkitoho", before)
        Programme.discipline = "aikido"
    }

    // Shinki Toho Shoden: the Kamae exercise is one row; its sheet lists the 26 positions.
    function test_2g_kamae_sheet() {
        failOnWarning(/ReferenceError|TypeError|Binding loop/)
        Programme.discipline = "shinkitoho"
        Programme.setGrade("shinkitoho", "shoden")
        const page = push("ProgrammePage")
        const kamae = page.rows.filter(r => r.kind === "entry" && r.id.startsWith("shinkitoho/kamae"))
        compare(kamae.length, 1)
        compare(kamae[0].id, "shinkitoho/kamae")
        page.open(kamae[0])
        const sheet = findChild(page, "techniqueSheet")
        tryVerify(() => sheet.opened)
        compare(sheet.steps.length, 26)
        const list = findChild(sheet.contentItem, "sheetSteps")
        tryCompare(list, "count", 26)
        const steps = findChildren(list, "step")
        compare(steps[2].modelData.title, "Seigan")
        compare(steps[11].modelData.title, "Seigan")
        shot("2g-sheet-kamae", true)
        sheet.close()
        tryVerify(() => !sheet.opened)
        // A technique without positions shows none.
        page.open(page.rows.find(r => r.kind === "entry" && r.id.startsWith("shinkitoho/odachi/")))
        tryVerify(() => sheet.opened)
        compare(sheet.steps.length, 0)
        verify(!findChild(sheet.contentItem, "sheetSteps").visible)
        sheet.close()
        tryVerify(() => !sheet.opened)
        window.pageStack.pop()
    }

    // A small phone: the Kamae sheet is taller than the window; the keys scroll it.
    function test_2h_sheet_keys() {
        Programme.discipline = "shinkitoho"
        Programme.setGrade("shinkitoho", "shoden")
        window.width = 360
        window.height = 640
        const page = push("ProgrammePage")
        page.open(page.rows.find(r => r.id === "shinkitoho/kamae"))
        const sheet = findChild(page, "techniqueSheet")
        tryVerify(() => sheet.opened)
        wait(500)
        const flick = sheet.flickable
        verify(flick && flick.contentHeight > flick.height, "taller than the window")
        compare(flick.contentY, 0)
        keyClick(Qt.Key_Down)
        tryVerify(() => flick.contentY > 0, 1000, "Down scrolls")
        const down = flick.contentY
        keyClick(Qt.Key_PageDown)
        tryVerify(() => flick.contentY > down, 1000, "Page Down scrolls")
        keyClick(Qt.Key_End)
        tryVerify(() => Math.abs(flick.contentY - (flick.contentHeight - flick.height)) < 2, 1000, "End to the last position")
        keyClick(Qt.Key_Home)
        tryCompare(flick, "contentY", 0)
        // With the focus on Close in the header (outside the content) too.
        const close = findCloseButton(sheet.header)
        verify(close, "the header's close button")
        close.forceActiveFocus()
        verify(close.activeFocus)
        keyClick(Qt.Key_PageDown)
        tryVerify(() => flick.contentY > 0, 1000, "Page Down scrolls from Close")
        sheet.close()
        tryVerify(() => !sheet.opened)
        window.pageStack.pop()
    }

    function findCloseButton(item) {
        if (!item) {
            return null
        }
        if (item.icon !== undefined && item.icon.name.startsWith("window-close") && item.visible) {
            return item
        }
        for (let i = 0; i < item.children.length; i++) {
            const found = findCloseButton(item.children[i])
            if (found) {
                return found
            }
        }
        return null
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

    // A comment with headings ("## Rei-ho …" in the Kumidachi text): headings, not raw marks;
    // the full title in the column (the header elides it).
    function test_3_comment_headings() {
        const all = []
        for (const g of Programme.grades("shinkitoho")) {
            for (const r of Programme.entries("shinkitoho", g.id)) {
                if (r.comment && r.comment.text && r.comment.text.indexOf("\n## ") >= 0) {
                    all.push(r)
                }
            }
        }
        verify(all.length > 0, "a comment with a heading")
        const page = push("CommentPage", { entry: all[0] })
        const headings = findChildren(page, "commentHeading")
        verify(headings.length > 0)
        verify(headings.every(h => !h.text.startsWith("#")), headings.map(h => h.text).join(" | "))
        verify(findChildren(page, "commentParagraph").every(p => !p.text.startsWith("#")))
        compare(findChild(page, "commentTitle").text, all[0].comment.titel)
        // A long title wraps, also where Kante draws it (Kante 1.68), never ends in "…".
        const long = Programme.entries("shinkitoho", Programme.grades("shinkitoho").find(g => Programme.entries("shinkitoho", g.id)
                         .some(r => r.comment && r.comment.titel.length > 60)).id).find(r => r.comment && r.comment.titel.length > 60)
        const longPage = push("CommentPage", { entry: long })
        const title = findChild(longPage, "commentTitle")
        // The text Kante draws over the hidden original: the child label with the same text.
        const drawn = Array.from(title.children).find(c => c.text === title.text && c.lineCount !== undefined)
        tryVerify(() => title.width <= longPage.width, 2000, "the title stays in the page: " + title.width)
        verify(!drawn || !drawn.visible || (!drawn.truncated && drawn.lineCount > 1), long.comment.titel)
        verify(!title.truncated)
        window.pageStack.pop()
        shot("4-comment-headings")
        window.pageStack.pop()
    }

    function test_3b_settings() {
        failOnWarning(/Binding loop/)       // the merge dialog measured its text against itself
        const page = push("SettingsPage")
        compare(page.title, "Settings")
        // The style's ground, not the platform's (ScrollablePage does not inherit the theme).
        verify(Qt.colorEqual(page.background.color, Look.backgroundColor),
               page.background.color + " is not " + Look.backgroundColor)
        // and the scope reaches the page's controls: the style's text colour, not the platform's.
        verify(Qt.colorEqual(page.Kirigami.Theme.textColor, Look.textColor),
               page.Kirigami.Theme.textColor + " is not " + Look.textColor)
        shot("4b-settings")
        window.pageStack.pop()
    }

    // Sync: switched on it shows the account; without one "Sync now" says what is missing. The
    // conflicts page without conflicts says so.
    function test_3d_sync() {
        const page = push("SettingsPage")
        verify(!findChild(page, "syncServer").visible)
        Sync.enabled = true
        tryVerify(() => findChild(page, "syncServer").visible)
        compare(findChild(page, "syncState").text, "Not synced yet.")
        verify(!Sync.syncNow())
        verify(findChild(page, "syncState").text.indexOf("server") >= 0, findChild(page, "syncState").text)
        shot("4d-sync")
        Sync.enabled = false
        window.pageStack.pop()

        const conflicts = push("ConflictsPage")
        compare(conflicts.title, "Conflicts")
        verify(findChild(conflicts, "noConflicts").visible)
        window.pageStack.pop()
    }

    // Backup: saved, read back on the same device it changes nothing; merging says so.
    function test_3c_backup() {
        const page = push("SettingsPage")
        const url = "file://" + uiShotsDir + "/sicherung-test.zip"
        verify(Backup.save(url), Backup.errorString)
        page.inspect(url)
        const dialog = findChild(page, "mergeBackup")
        tryVerify(() => dialog.opened)
        const text = findChild(page, "mergeText")
        verify(text.text.indexOf("Nothing new") >= 0, text.text)
        tryVerify(() => dialog.contentHeight >= text.implicitHeight, 1000,
                  "the text fits: " + dialog.contentHeight + " for " + text.implicitHeight)
        shot("4c-backup", true)
        dialog.accept()
        tryVerify(() => findChild(page, "backupDone").visible)
        compare(findChild(page, "backupError").visible, false)
        window.pageStack.pop()
    }

    // The player's switches: the screen stays on while a video plays (and the switch on);
    // sound off mutes also at full tempo; both stay for the next start. "Noted today" lists
    // the day's notes on the technique.
    function test_4t_player_switches() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        const page = push("PlayerPage", { playlist: rows, index: 0 })
        verify(!Levels.held, "no video plays: the screen may sleep")
        Levels.setPlayback(Levels.Playback.Playing)     // as the player does while it plays
        const awake = findChild(page, "awakeSwitch")
        const sound = findChild(page, "soundSwitch")
        verify(awake.checked && sound.checked)
        awake.clicked()
        verify(!Levels.held && !Levels.awake)
        awake.clicked()
        verify(Levels.held)
        sound.clicked()
        verify(!Levels.sound && !sound.checked)
        sound.clicked()
        verify(Levels.sound)
        findChild(page, "transportBar").speedChangeRequested(0.5)
        verify(!sound.checked, "slow motion is silent")
        verify(!sound.enabled, "no unseen change in slow motion")
        findChild(page, "transportBar").speedChangeRequested(1)
        verify(sound.checked)

        const noted = findChild(page, "notedToday")
        verify(!noted.visible)
        Practice.addNote(page.current.id, "Schritt kleiner")
        Practice.addNote(page.current.id, "Blick")
        verify(noted.visible)
        verify(noted.text.indexOf("Schritt kleiner") >= 0 && noted.text.indexOf("Blick") >= 0, noted.text)
        for (const n of Practice.notes(page.current.id)) {
            Practice.deleteNote(n.uid)
        }
        window.pageStack.pop()
        tryVerify(() => !Levels.held, 2000, "the screen may sleep again")
    }

    // Swipe on the picture: up on the left half brightens the screen, down on the right half
    // turns the sound down; leaving the player gives the screen its brightness back. The levels
    // stay in memory (MITORI_DEVICE_LEVELS), no video needed.
    function test_4s_player_swipe() {
        failOnWarning(/ReferenceError|TypeError|Binding loop/)
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        const page = push("PlayerPage", { playlist: rows, index: 0 })
        const stage = findChild(page, "stage")
        verify(stage)
        stage.visible = true    // without a video a card stands for the stage; the gesture needs it
        wait(800)       // Kirigami slides the page in (disabled meanwhile)
        verify(stage.height > 100, "stage " + stage.width + "×" + stage.height)
        const brightness = Levels.brightness
        const volume = Levels.volume
        mouseDrag(stage, stage.width / 4, stage.height * 0.8, 0, -stage.height / 2)
        tryVerify(() => Levels.brightness > brightness + 0.3, 2000, "brightness " + Levels.brightness)
        compare(Levels.volume, volume)
        mouseDrag(stage, stage.width * 3 / 4, stage.height * 0.2, 0, stage.height / 4)
        tryVerify(() => Levels.volume < volume - 0.1, 2000, "volume " + Levels.volume)
        verify(!findChild(page, "levelShown").visible, "shown only while swiping")
        window.pageStack.pop()
        tryCompare(Levels, "brightness", brightness)
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
        verify(Qt.colorEqual(page.background.color, Look.backgroundColor), "the style's ground")
        shot("5-player")
        window.width = 900
        window.height = 420
        wait(500)
        shot("6-player-landscape")
        // A phone held sideways: Back in view without scrolling the column.
        window.width = 800
        window.height = 360
        wait(500)
        const view = findChild(page, "controlsView")
        const back = findChild(page, "backButton")
        verify(view && back)
        verify(back.visible)
        compare(view.contentY, 0)
        const at = back.mapToItem(view, 0, 0)
        verify(at.y >= 0 && at.y + back.height <= view.height + 1, "Back in view (" + at.y + ", " + view.height + ")")
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
        verify(Qt.colorEqual(page.background.color, Look.backgroundColor), "the style's ground")
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

    // Training without videos (CI): every due technique comes in, shown as a card, with the
    // playlist (✓, tap jumps), the position and an end hint with the count.
    function test_7c_training_list() {
        Programme.discipline = "daitoryu"
        Programme.setGrade("daitoryu", "k4")
        const today = push("TodayPage")
        verify(today.due.length >= 2, "Daitoryu 4. Kyu has due techniques")
        today.startTraining()
        tryVerify(() => window.pageStack.currentItem.training === true)
        const player = window.pageStack.currentItem
        compare(player.playlist.length, today.due.length, "every due technique, with or without a video")

        // Position and the card standing for a missing video.
        const place = findChild(player, "placeText")
        verify(place.text.endsWith("1 / " + player.playlist.length), place.text)
        verify(place.text.indexOf(player.current.gradeName) >= 0, place.text)
        if (!player.hasVideo) {
            verify(findChild(player, "noVideoCard").visible)
            verify(!findChild(player, "stage").visible)
        }

        // The playlist: one numbered row each, the current one selected, a tap jumps.
        const rows = findChildren(player, "trainingRow")
        compare(rows.length, player.playlist.length)
        verify(rows[0].selected)
        verify(rows[1].text.startsWith("2. "), rows[1].text)
        rows[1].clicked()
        compare(player.index, 1)
        verify(rows[1].selected && !rows[0].selected)
        verify(place.text.endsWith("2 / " + player.playlist.length), place.text)
        shot("7c-training-list")

        // Rating ticks the row; all rated: the end hint counts them.
        const before = player.practisedCount
        findPicker(player, player.current.id).picked(3)
        verify(rows[1].done)
        compare(player.practisedCount, before + 1)
        for (let i = 0; i < player.playlist.length; ++i) {
            if (!Practice.ratedToday(player.playlist[i].id)) {
                Practice.rate(player.playlist[i].id, 2)
            }
        }
        verify(player.trainingDone)
        verify(findChild(player, "trainingDone").text.indexOf(String(player.playlist.length)) >= 0)
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
        // Tab to a note: the style's focus frame (Shinki: 2 px ink, not red, which means chosen).
        note.forceActiveFocus(Qt.TabFocusReason)
        if (uiShinki) {
            verify(Qt.colorEqual(note.background.border.color, Look.textColor), "focus in ink: " + note.background.border.color)
            verify(note.background.border.width >= 2, "focus 2 px: " + note.background.border.width)
        }
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

    // The sheet from the search: "Enter code" leads to the videos (as from the programme).
    function test_9_search_sheet_code() {
        const page = push("SearchPage")
        findChild(page, "searchField").text = "ikkyo shomen"
        tryVerify(() => page.techniques.length > 0, 2000)
        const result = page.techniques.find(r => !!r.video && !Videos.available(r.video.video))
        if (!result) {
            skip("every Ikkyo video is on this device")
        }
        const sheet = findChild(page, "techniqueSheet")
        sheet.entry = result
        sheet.playable = false
        sheet.open()
        tryVerify(() => sheet.opened)
        findChild(sheet.contentItem, "sheetCodeButton").clicked()
        tryVerify(() => window.pageStack.currentItem.title === "Videos", 2000, "the videos page opens")
        window.pageStack.pop()
        window.pageStack.pop()
    }

    // Search: techniques across disciplines and own notes; a result opens the sheet.
    function test_9_search() {
        Practice.addNote(Programme.entries("aikido", "k5").find(r => r.kind === "entry").id, "Kreis größer machen")
        const page = push("SearchPage")
        const field = findChild(page, "searchField")
        field.text = "ikkyo shomen"
        tryVerify(() => page.techniques.length > 0, 2000)
        verify(page.techniques.every(r => r.discipline !== undefined))
        compare(page.notes.length, 0)
        field.text = "kreis grosser"                    // größer without umlaut and ß
        tryVerify(() => page.query === "kreis grosser", 2000)
        compare(page.notes.length, 1)
        // More than the list shows: a hint to narrow the search.
        field.text = "nage"
        tryVerify(() => page.query === "nage", 2000)
        verify(page.more, "more than the list shows")
        compare(page.shown.length, page.limit)
        verify(findChild(page, "moreHint").visible)
        // The notes have the same limit and hint.
        for (let i = 0; i <= page.limit; i++)
            Practice.addNote("many-notes", "Wiederholung " + i)
        field.text = "wiederholung"
        tryVerify(() => page.query === "wiederholung", 2000)
        verify(page.notesMore)
        compare(page.notesShown.length, page.limit)
        verify(findChild(page, "notesMoreHint").visible)
        // A note on a technique that is no longer in the programme still opens its notes.
        Practice.addNote("gone-technique", "Kreis kleiner")
        field.text = "kreis kleiner"
        tryVerify(() => page.query === "kreis kleiner", 2000)
        compare(page.notes.length, 1)
        const orphan = findChild(page, "noteResult")
        mouseClick(orphan, orphan.width / 3, orphan.height / 2)
        tryVerify(() => window.pageStack.currentItem !== page, 2000)
        compare(window.pageStack.currentItem.entry.id, "gone-technique")
        verify(window.pageStack.currentItem.notes.length === 1)
        window.pageStack.pop()
        field.text = "ikkyo"
        tryVerify(() => page.query === "ikkyo", 2000)
        verify(!page.more)
        shot("9-search")
        const result = findChild(page, "techniqueResult")
        mouseClick(result, result.width / 3, result.height / 2)
        const sheet = findChild(page, "techniqueSheet")
        tryVerify(() => sheet.opened, 2000)
        sheet.close()
        tryVerify(() => !sheet.opened)
        window.pageStack.pop()
    }

    // Courses: the coming dates from the calendar (a local file in this test), a date's notebook
    // with an entry, a course of one's own, deleting a notebook.
    function test_9m_courses() {
        failOnWarning(/ReferenceError|TypeError|Binding loop/)
        tryCompare(window.pageStack, "depth", 1)
        wait(500)
        const tile = findChild(window.pageStack.items[0], "coursesTile")
        verify(tile && tile.visible)
        clickTile(tile)
        tryCompare(window.pageStack, "depth", 2)
        const page = window.pageStack.currentItem
        tryVerify(() => page.title === qsTr("Courses"))
        tryCompare(Courses, "calendarState", Courses.CalendarState.Ready, 5000)
        tryVerify(() => findChildren(page, "eventRow").length === 2)
        const rows = findChildren(page, "eventRow")
        compare(rows[0].course.title, "Daitoryu Aiki Jujutsu")
        compare(rows[0].course.teacher, "M. D. Nakajima Shihan")
        compare(rows[0].daysAway, 12)
        compare(rows[1].daysAway, 40)
        // The first day as a local day (no time zone moves it to the day before).
        compare(rows[0].firstDay.getDate(), new Date(Date.now() + 12 * 86400000).getDate())
        compare(findChildren(page, "notebookRow").length, 0)
        verify(findChild(window.pageStack.items[0], "coursesTile").hint.indexOf("Siebeldingen") === 0)
        shot("14-courses")

        // A date opens its notebook, made on first use; an entry is added from the field.
        mouseClick(rows[0])
        tryCompare(window.pageStack, "depth", 3)
        const book = window.pageStack.currentItem
        tryVerify(() => book.title === qsTr("Notebook"))
        compare(findChild(book, "courseTitle").text, "Daitoryu Aiki Jujutsu")
        verify(findChild(book, "announcement").visible, "the calendar names an announcement")
        verify(findChild(book, "toCalendar").visible)
        const compose = findChild(book, "courseCompose")
        compose.text = "Kote-gaeshi aus Tsuki"
        compose.accepted()                              // Enter in the field
        tryVerify(() => book.entries.length === 1)
        compare(compose.text, "")
        tryVerify(() => findChildren(book, "courseEntry").length === 1)
        shot("14b-course-notebook")
        window.pageStack.pop()
        tryVerify(() => window.pageStack.currentItem === page)
        tryVerify(() => findChildren(page, "notebookRow").length === 1)
        compare(findChildren(page, "eventRow")[0].course.entries, 1)
        compare(Courses.notebookFor("ui-daito"), book.uid, "the same notebook again")

        // A course of one's own.
        const made = push("CourseNewPage")
        verify(!findChild(made, "makeCourse").enabled, "a title is needed")
        findChild(made, "courseTitleField").text = "Training mit Gastlehrer"
        findChild(made, "coursePlace").text = "Dojo Weimar"
        tryVerify(() => findChild(made, "makeCourse").enabled)
        mouseClick(findChild(made, "makeCourse"))
        tryVerify(() => window.pageStack.currentItem !== made && window.pageStack.currentItem.uid !== undefined)
        const own = window.pageStack.currentItem
        compare(own.course.title, "Training mit Gastlehrer")
        compare(own.course.place, "Dojo Weimar")
        verify(!findChild(own, "announcement").visible)
        // Deleting it asks first.
        const remove = findChild(own, "removeCourse")
        remove.open()
        tryVerify(() => remove.opened)
        shot("14c-course-delete", true)
        remove.accept()
        tryVerify(() => window.pageStack.currentItem === page, 3000)
        compare(findChildren(page, "notebookRow").length, 1)
        window.pageStack.pop()
    }

    // Videos: a DVD's code unlocks its bundle (the test data's), which loads onto the device and
    // plays from there; a wrong code says why; removing keeps the code.
    function test_9n_videos() {
        failOnWarning(/ReferenceError|TypeError|Binding loop/)
        const page = push("VideosPage")
        tryCompare(Bundles, "manifestState", Bundles.ManifestState.Ready, 5000)
        tryVerify(() => findChildren(page, "bundleRow").length === 1)
        const row = () => findChildren(page, "bundleRow")[0]
        compare(row().bundleState, Bundles.State.Locked)
        // A change of the list changes the row in place (a press on Cancel during a download
        // meets the same button when it is let go).
        const first = row()
        verify(!findChild(row(), "bundleAction").visible)
        const field = findChild(page, "codeField")
        const message = findChild(page, "codeMessage")
        field.text = "7KQ2M-X9HDV-3TAPR-WE6YA"          // check character wrong
        wait(800)       // Kirigami slides the page in (disabled meanwhile)
        mouseClick(findChild(page, "unlockButton"))
        tryVerify(() => message.visible)
        compare(message.text, qsTr("This code has a typing error; check it character by character."))
        shot("15-videos-code-wrong")
        field.text = "7kq2m x9hdv 3tapr we6yt"
        mouseClick(findChild(page, "unlockButton"))
        tryVerify(() => Bundles.bundles[0].state === Bundles.State.Installed, 10000)
        verify(message.text.indexOf("Test") > 0, message.text)
        compare(field.text, "")
        verify(Videos.available("test/v"), "the bundle's video is on the device")
        compare(Videos.crop("test/v").h, undefined, "a bundle's file is cut already")
        verify(Bundles.size > 0)
        compare(findChildren(page, "bundleRow").length, 1)
        verify(row() === first, "the row stays through unlocking and loading")
        tryVerify(() => findChild(row(), "bundleAction").text === qsTr("Remove"))
        shot("15b-videos-loaded")
        mouseClick(findChild(row(), "bundleAction"))
        tryVerify(() => Bundles.bundles[0].state === Bundles.State.Unlocked)
        verify(!Videos.available("test/v"))
        compare(Bundles.bundles[0].code, "7KQ2M-X9HDV-3TAPR-WE6YT", "the code stays")
        window.pageStack.pop()
    }

    // My grades and a planned exam: a passed grade sets the next one and its waiting time; an
    // exam planned on a coming course shows in the start menu, today's practice and the exam mode.
    function test_9o_grades() {
        failOnWarning(/ReferenceError|TypeError|Binding loop/)
        // grades.js names the states by number.
        compare([Courses.Wait.First, Courses.Wait.FromStart, Courses.Wait.Unknown, Courses.Wait.Earliest,
                 Courses.Wait.Fits, Courses.Wait.TooEarly], [0, 1, 2, 3, 4, 5])
        const page = push("GradesPage")
        tryVerify(() => findChildren(page, "addGrade").length === 3)
        const aikido = findChild(page, "grades_aikido")
        verify(findChild(aikido, "nextGrade").text.indexOf(qsTr("Next: %1").arg("5. Kyu")) === 0)
        findChild(aikido, "gradeDay").date = new Date(Date.now() - 400 * 86400000)
        findChild(aikido, "gradePlace").text = "Mainz"
        const add = findChild(aikido, "addGrade")
        // Kirigami slides the page in, disabled meanwhile (longer under load than 800 ms).
        wait(800)
        tryVerify(() => add.enabled, 5000)
        const at = add.mapToItem(page, 0, 0)
        mouseClick(add)
        tryVerify(() => Courses.grades.length === 1, 3000,
                  "add at " + at.x + "," + at.y + " enabled " + add.enabled + " page " + page.width + "×" + page.height)
        tryVerify(() => findChildren(aikido, "passedRow").length === 1)
        compare(findChild(aikido, "gradePlace").text, "")
        const next = findChild(aikido, "nextGrade").text
        verify(next.indexOf(qsTr("Next: %1").arg("4. Kyu")) === 0, next)
        compare(findChild(aikido, "gradeBox").currentIndex, 1, "the next grade is offered")
        // Shinki Toho Shoden counts from the start of training: entered, its waiting time has a day.
        const toho = findChild(page, "grades_shinkitoho")
        verify(findChild(toho, "trainingStart").visible)
        verify(!findChild(aikido, "trainingStart").visible)
        verify(Courses.setTrainingStart("shinkitoho", "2024-09-01"))
        tryVerify(() => findChild(toho, "trainingStart").date !== null)
        const shoden = findChild(toho, "nextGrade").text
        verify(shoden.indexOf("enter it") < 0 && /26/.test(shoden), shoden)     // from 01.09.26 (C: 1 09 2026)
        // Picked by the user, the field still follows the saved day (a sync may bring another).
        const startField = findChild(toho, "trainingStart")
        startField.pick(new Date(2024, 9, 1, 12))
        compare(Courses.trainingStart("shinkitoho"), "2024-10-01")
        verify(Courses.setTrainingStart("shinkitoho", "2024-09-01"))
        tryVerify(() => startField.date && startField.date.getMonth() === 8, 2000, "the field follows the saved day")
        shot("16-grades")
        window.pageStack.pop()

        // An exam on the coming Daitoryu course (in 12 days): 4. Kyu fits (a year after 5. Kyu).
        const uid = Courses.notebookFor("ui-daito")
        const book = push("CoursePage", { uid: uid })
        verify(findChild(book, "planExam").visible)
        wait(800)
        mouseClick(findChild(book, "planExam"))
        book.examDiscipline = "aikido"
        book.examGrade = Courses.nextGrade("aikido")
        tryVerify(() => findChild(book, "examGrade_k4") && findChild(book, "examGrade_k4").checked)
        verify(findChild(book, "examGrade_k4").text.indexOf(qsTr("next")) > 0)
        verify(findChild(book, "examGrade_k5").text.indexOf(qsTr("passed")) > 0)
        verify(findChild(book, "choiceWait").text.indexOf(qsTr("fits (from %1)").split("%1")[0]) > 0, findChild(book, "choiceWait").text)
        shot("16b-course-exam-choose")
        mouseClick(findChild(book, "examPlan"))
        tryVerify(() => findChild(book, "examChip").visible)
        compare(findChild(book, "examChip").text, qsTr("Exam %1").arg("Aikido 4. Kyu"))
        shot("16c-course-exam")
        window.pageStack.pop()

        const start = window.pageStack.items[0]
        tryVerify(() => findChild(start, "coursesTile").hint === qsTr("Exam %1 in %n day(s)", "", 12).arg("Aikido 4. Kyu"))
        Programme.discipline = "aikido"
        const today = push("TodayPage")
        verify(findChild(today, "plannedHint").visible)
        window.pageStack.pop()
        Programme.setGrade("aikido", "k5")
        const exam = push("ExamPage")
        verify(findChild(exam, "plannedHint").visible)
        verify(findChild(exam, "toExamGrade").visible)
        wait(800)
        mouseClick(findChild(exam, "toExamGrade"))
        tryCompare(exam, "grade", "k4")
        verify(!findChild(exam, "toExamGrade").visible)
        window.pageStack.pop()

        // A planned grade without material in the exam mode (Shinki Toho Menkyo-Kaiden): no
        // button that would open another grade.
        verify(Courses.planExam(uid, "shinkitoho", "kaiden"))
        Programme.discipline = "shinkitoho"
        const today2 = push("TodayPage")
        verify(findChild(today2, "plannedHint").visible)
        verify(!findChild(today2, "toMockExam").visible, "Menkyo-Kaiden has no mock exam")
        window.pageStack.pop()
        const exam2 = push("ExamPage")
        verify(findChild(exam2, "plannedHint").visible)
        verify(!findChild(exam2, "toExamGrade").visible, "Menkyo-Kaiden has no mock exam")
        window.pageStack.pop()
        Programme.discipline = "aikido"

        // Back as before for the other cases.
        verify(Courses.planExam(uid, "", ""))
        verify(Courses.removeGrade(Courses.grades[0].uid))

        // Every grade passed: nothing is chosen, so "Add" cannot overwrite the first grade's day;
        // the heading names the highest grade, also when a lower one has the later day.
        const ladder = Courses.ladder("daitoryu")
        ladder.forEach((g, i) => verify(Courses.addGrade("daitoryu", g.id, (2010 + i) + "-01-01", "")))
        verify(Courses.addGrade("daitoryu", "k5", "2030-01-01", ""))
        const all = push("GradesPage")
        const daito = findChild(all, "grades_daitoryu")
        tryVerify(() => findChild(daito, "addGrade"))
        compare(findChild(daito, "gradeBox").currentIndex, -1)
        verify(!findChild(daito, "addGrade").enabled)
        verify(findChild(daito, "gradesHeading").text.indexOf(ladder[ladder.length - 1].name) > 0, findChild(daito, "gradesHeading").text)
        window.pageStack.pop()
        const uids = Courses.grades.filter(g => g.discipline === "daitoryu").map(g => g.uid)
        compare(uids.length, ladder.length)
        for (const u of uids) {
            verify(Courses.removeGrade(u), "remove " + u + ": " + Courses.errorString)
        }
    }

    // Technique map: from the start menu, a grid per group with empty places, a place coloured by
    // its level and opening its sheet, the keys moving on; Shinki Toho as numbered forms.
    function test_9k_map() {
        failOnWarning(/ReferenceError|TypeError/)
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k4")
        tryCompare(window.pageStack, "depth", 1)
        wait(500)       // Kirigami slides the last page out
        const tile = findChild(window.pageStack.items[0], "mapTile")
        verify(tile && tile.visible)
        clickTile(tile)
        tryCompare(window.pageStack, "depth", 2)
        const page = window.pageStack.currentItem
        tryVerify(() => page.title === qsTr("Technique map"))
        wait(500)       // Kirigami slides the page in
        compare(page.grade, "k4")
        verify(page.matrix)
        compare(findChildren(page, "gradeChip").length, Programme.grades("aikido").length)

        // Techniques × attacks (KanteHeatGrid); combinations not in the programme are gaps.
        const heat = findChild(page, "techniqueMatrix")
        compare(heat.rowCount, page.shown.rows.length)
        compare(heat.columnCount, page.shown.columns.length)
        // Technique names in the text font, not the small mono labels.
        compare(heat.rowLabelFont.family, KanteStyle.smallFont.family)
        compare(heat.rowLabelFont.pointSize, KanteStyle.smallFont.pointSize)
        let gap = null, target = null
        for (let r = 0; r < heat.rowCount; r++) {
            for (let c = 0; c < heat.columnCount; c++) {
                if (heat.isEmpty(r, c))
                    gap = gap || { r: r, c: c }
                else if (!target && heat.valueOf(r, c) === 0)
                    target = { r: r, c: c }
            }
        }
        verify(gap, "combinations not in the programme stay empty")
        verify(!heat.cellAt(gap.r, gap.c).shown)
        verify(target)

        // A rating colours the place; its tip names technique, attack, grade and level.
        const data = page.shown.rows[target.r].cells[target.c]
        verify(Practice.rate(data.ids[0], 4))
        const cell = heat.cellAt(target.r, target.c)
        tryCompare(cell, "level", 4)
        compare(heat.entryOf(target.r, target.c).label, "4")
        const tip = heat.tipOf(target.r, target.c)
        verify(tip.indexOf(data.gradeName) > 0, tip)
        verify(tip.indexOf(Practice.levelNames[4]) > 0, tip)
        shot("11-map-aikido")

        // A tap opens the technique sheet.
        const sheet = findChild(page, "techniqueSheet")
        mouseClick(cell)
        tryVerify(() => sheet.opened)
        compare(sheet.entry.id, data.entry.id)
        sheet.close()
        tryVerify(() => !sheet.visible)

        // The keys: Right moves to the next place of the row (over gaps), Enter opens it.
        cell.forceActiveFocus(Qt.TabFocusReason)
        let next = target.c + 1
        while (next < heat.columnCount && heat.isEmpty(target.r, next))
            next++
        keyClick(Qt.Key_Right)
        if (next < heat.columnCount)
            compare(heat.currentColumn, next)
        keyClick(Qt.Key_Return)
        tryVerify(() => sheet.opened)
        compare(sheet.entry.id, page.shown.rows[heat.currentRow].cells[heat.currentColumn].entry.id)
        sheet.close()
        tryVerify(() => !sheet.visible)       // a closing sheet still takes the taps

        // Groups apart: another chip, another grid.
        const chips = findChildren(page, "groupChip")
        verify(chips.length > 1)
        page.flickable.contentY = 0
        mouseClick(chips[1])
        compare(page.shown.title, chips[1].text)

        // Shinki Toho: the Ô-dachi forms up to Chûden, eight to a row on the phone.
        page.discipline = "shinkitoho"
        page.grade = "chuden"
        verify(!page.matrix)
        const forms = findChild(page, "formGrid")
        verify(forms.visible)
        compare(forms.perRow, 8)
        compare(forms.rowCount, 3)
        // The forms fill the width: the last of a row ends at the right edge.
        const fills = () => {
            const last = forms.cellAt(0, forms.perRow - 1)
            const right = last.mapToItem(forms, last.width, 0).x
            return right <= forms.width + 0.5 && right > forms.width - forms.cellSide / 2
        }
        tryVerify(fills, 2000, "the forms fill the width")
        compare(forms.entryOf(1, 3).label, "12")
        verify(forms.isEmpty(2, 7), "after the last form")
        verify(Practice.rate("shinkitoho/odachi/12", 2))
        const form = forms.cellAt(1, 3)
        tryCompare(form, "level", 2)
        // The level also as a number, not only as the colour.
        compare(forms.entryOf(1, 3).label, "12·2")
        waitForRendering(forms)       // the page lays out the forms in the next frame
        mouseClick(form)
        tryVerify(() => sheet.opened)
        compare(sheet.entry.id, "shinkitoho/odachi/12")
        sheet.close()
        tryVerify(() => !sheet.visible)
        shot("11-map-shinkitoho")

        // Tablet in landscape: bigger places, more forms per row.
        window.width = 1280
        window.height = 800
        tryCompare(forms, "perRow", 11)
        compare(forms.entryOf(1, 0).label, "12·2")
        tryVerify(fills, 2000, "the forms fill the tablet's width")
        page.discipline = "daitoryu"
        page.grade = Programme.grades("daitoryu").slice(-1)[0].id
        verify(page.matrix)
        compare(heat.cellSize, KanteStyle.unit(34))
        // A place with entries of several grades names them all.
        let multi = null
        for (let r = 0; !multi && r < heat.rowCount; r++) {
            for (let c = 0; !multi && c < heat.columnCount; c++) {
                const d = page.shown.rows[r].cells[c]
                if (d.gradeNames && d.gradeNames.length > 1)
                    multi = { r: r, c: c, names: d.gradeNames }
            }
        }
        verify(multi, "a place with entries of two grades in Daitoryu")
        for (const name of multi.names)
            verify(heat.tipOf(multi.r, multi.c).indexOf(name) > 0, heat.tipOf(multi.r, multi.c))
        shot("11-map-tablet")
        window.pageStack.pop()
    }

    // Glossary: without a query A–Z with headings, the source below; a query finds words by
    // romaji, kanji and German next to techniques; without a Japanese voice no speaker buttons.
    SignalSpy {
        id: saySpy

        target: Glossary
        signalName: "sayRequested"
    }

    function test_9b_glossary() {
        failOnWarning(/ReferenceError|TypeError/)
        const start = window.pageStack.items[0]
        const tile = findChild(start, "glossaryTile")
        verify(tile.visible)
        // Cases that end with skip() (no videos) leave their pages on the stack.
        while (window.pageStack.depth > 1)
            window.pageStack.pop()
        tryVerify(() => window.pageStack.currentItem === start)
        wait(800)       // Kirigami slides the start page back in; a click meanwhile goes nowhere
        clickTile(tile)
        tryVerify(() => window.pageStack.currentItem !== start && window.pageStack.currentItem.glossary !== undefined, 2000)
        const page = window.pageStack.currentItem
        const list = findChild(page, "glossaryList")
        const field = findChild(page, "searchField")
        verify(Glossary.words.length > 219)
        tryCompare(list, "count", Glossary.words.length)
        verify(findChild(page, "glossaryIntro").visible)
        compare(list.section.property, "letter")
        tryVerify(() => findChild(list, "letterHeading") !== null)
        compare(findChild(list, "letterHeading").text, "A")
        compare(Glossary.words[0].word, "Ago")
        shot("9b-glossary")

        // No Japanese voice (mock engine): a hint, no buttons, saying does nothing.
        verify(Glossary.speech !== Glossary.Ready)
        verify(findChild(page, "speechHint").visible)
        const say = findChild(list, "sayButton")
        verify(say === null || !say.visible)
        Glossary.say(Glossary.words[0].speech)

        // Keyboard: Down from the field into the list, Return and Space say the current word,
        // Up back to the field.
        field.forceActiveFocus()
        keyClick(Qt.Key_Down)
        tryVerify(() => list.activeFocus)
        compare(list.currentIndex, 0)
        keyClick(Qt.Key_Down)
        compare(list.currentIndex, 1)
        saySpy.clear()
        keyClick(Qt.Key_Return)
        compare(saySpy.count, 1)
        compare(saySpy.signalArguments[0][0], page.glossary[1].speech)
        keyClick(Qt.Key_Space)
        compare(saySpy.count, 2)
        keyClick(Qt.Key_Up)
        keyClick(Qt.Key_Up)
        tryVerify(() => field.activeFocus)

        // The same word by romaji with or without hyphen and by kanji with other okurigana.
        for (const q of ["Kiriotoshi", "Kiri-otoshi", "切り落とし"]) {
            field.text = q
            tryVerify(() => page.query === q, 2000)
            verify(page.glossary.some(w => w.word === "Kiri-otoshi"), q)
            verify(page.techniques.length > 0, q)
        }
        compare(list.section.property, "")
        verify(!findChild(page, "glossaryIntro").visible)
        // German words find their Japanese term.
        field.text = "hüftwurf"
        tryVerify(() => page.query === "hüftwurf", 2000)
        verify(page.glossary.some(w => w.word === "Koshi-nage"))
        tryCompare(list, "count", page.glossary.length)
        tryVerify(() => findChild(page, "glossarySource") !== null && findChild(page, "glossarySource").visible)
        verify(findChild(page, "glossarySource").text.includes("shinkiryu.de"))
        shot("9b-glossary-search")
        window.pageStack.pop()
    }

    // All notes: every technique's notes by day, a filter per discipline; a tap opens the
    // technique's notes, press and hold the editor.
    function test_8d_all_notes() {
        failOnWarning(/ReferenceError|TypeError/)
        const daito = Programme.entries("daitoryu", Programme.grades("daitoryu")[0].id).find(r => r.kind === "entry")
        Practice.addNote(daito.id, "Daitoryu: Hüfte zuerst")
        Practice.addNote("not-in-the-programme", "verwaist")
        const page = push("AllNotesPage")
        page.filter = ""
        const total = page.shown.length
        verify(total >= 2)
        compare(page.shown[0].text, "verwaist", "newest first")
        verify(page.shown.filter(n => n.day === page.shown[0].day).length >= 2, "both today")
        wait(500)       // Kirigami slides the page in
        mouseClick(findChild(page, "filter_daitoryu"))
        tryVerify(() => page.filter === "daitoryu")
        verify(page.shown.length >= 1 && page.shown.every(n => n.discipline === "daitoryu"))
        verify(page.shown.length < total, "the orphan and other disciplines left out")
        shot("8d-all-notes")
        const note = findChild(page, "note")
        mouseClick(note, 20, note.height - 10)
        tryVerify(() => window.pageStack.currentItem !== page, 2000)
        compare(window.pageStack.currentItem.entry.id, daito.id)
        window.pageStack.pop()
        tryVerify(() => window.pageStack.currentItem === page)
        mousePress(note, 20, note.height - 10)
        wait(1000)
        mouseRelease(note, 20, note.height - 10)
        const editor = findChild(page, "noteEditor")
        tryVerify(() => editor.opened, 2000, "press and hold opens the editor")
        editor.close()
        tryVerify(() => !editor.opened)
        // Many notes: a virtual list builds delegates for the visible ones only.
        page.filter = ""
        wait(50)        // another filter goes to the top a moment later
        const many = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        for (let i = 0; i < 60; i++)
            Practice.addNote(many[i % many.length].id, "Übung " + i)
        const list = findChild(page, "notesList")
        tryVerify(() => list.count >= 60)
        verify(findChildren(list.contentItem, "note").length < list.count / 2, "only the visible delegates")
        // A change further down keeps the place in the list.
        list.positionViewAtIndex(40, ListView.Beginning)
        wait(50)
        const top = page.shown[list.indexAt(10, list.contentY + 10)].uid
        Practice.changeNote(page.shown[42].uid, "geändert")
        tryVerify(() => page.shown[42].text === "geändert")
        wait(50)
        compare(page.shown[list.indexAt(10, list.contentY + 10)].uid, top, "same note at the top")
        // Also with the gap between two notes at the top, and when the top note is deleted.
        const y = list.contentY
        list.contentY = y - list.spacing / 2
        wait(50)
        verify(list.indexAt(10, list.contentY) < 0, "a gap at the very top")
        Practice.changeNote(page.shown[44].uid, "noch einmal")
        wait(50)
        compare(page.shown[list.indexAt(10, list.contentY + 10)].uid, top, "same note after a gap at the top")
        const next = page.shown[41].uid
        Practice.deleteNote(top)
        tryVerify(() => !page.shown.some(n => n.uid === top))
        wait(50)
        compare(page.shown[list.indexAt(10, list.contentY + list.spacing + 10)].uid, next, "the next note moves up")
        // Tab: through the filter to the list, on a note in view (not the first one far above).
        mouseClick(findChild(page, "filter_all"))
        const chips = Programme.disciplines.length + 2          // All, the disciplines, Courses
        for (let i = 0; i <= chips + 2 && !list.activeFocus; i++)       // Back first if the page has the focus
            keyClick(Qt.Key_Tab)
        verify(list.activeFocus, "Tab reaches the list after the filter")
        keyClick(Qt.Key_Backtab)
        compare(window.activeFocusItem, findChild(page, "filter_courses"), "Shift+Tab goes back to the filter")
        keyClick(Qt.Key_Tab)
        verify(list.activeFocus)
        // A whole round of Tab: the toolbar (Back), every chip and the list, each once.
        const round = []
        for (let i = 0; i < 30; i++) {
            keyClick(Qt.Key_Tab)
            if (list.activeFocus)
                break
            round.push(String(window.activeFocusItem))
        }
        verify(list.activeFocus, "back at the list: " + round.join(" > "))
        verify(round.some(t => t.indexOf("NavButton") >= 0), "Tab reaches Back: " + round.join(" > "))
        compare(round.filter(t => t.indexOf("filter_") >= 0).length, chips, round.join(" > "))
        const cur = list.itemAtIndex(list.currentIndex)
        verify(cur && cur.y + cur.height > list.contentY && cur.y < list.contentY + list.height,
               "the current note is in view (" + list.currentIndex + ")")
        // Changed with the keyboard: the focus and the note stay.
        const at = list.currentIndex
        keyClick(Qt.Key_Menu)
        tryVerify(() => editor.opened, 2000)
        Practice.changeNote(page.shown[at].uid, "mit der Tastatur")
        editor.close()
        tryVerify(() => !editor.opened)
        tryVerify(() => list.activeFocus, 1000, "the list has the focus again")
        compare(list.currentIndex, at)
        // Page Down, End and Home move through the list.
        keyClick(Qt.Key_PageDown)
        verify(list.currentIndex > at + 1, "Page Down: " + list.currentIndex)
        keyClick(Qt.Key_End)
        compare(list.currentIndex, list.count - 1)
        keyClick(Qt.Key_Home)
        compare(list.currentIndex, 0)
        tryVerify(() => list.atYBeginning, 1000, "Home shows the count above the first note")
        // Page Up back to the first note shows the count too.
        for (let i = 0; i < 3; i++)
            keyClick(Qt.Key_PageDown)
        while (list.currentIndex > 0)
            keyClick(Qt.Key_PageUp)
        tryVerify(() => list.atYBeginning, 1000, "Page Up to the first note shows the count")
        // A new note while the current one is out of view: the view stays where it is.
        list.positionViewAtIndex(30, ListView.Beginning)
        wait(50)
        const shownTop = page.shown[list.indexAt(10, list.contentY + 10)].uid
        Practice.addNote(many[0].id, "ganz neu")
        tryVerify(() => page.shown[0].text === "ganz neu")
        wait(50)
        compare(page.shown[list.indexAt(10, list.contentY + 10)].uid, shownTop, "no jump to the current note")
        // Scrolled, the notes stay below the filter (the list is clipped).
        const chip = findChild(page, "filter_all")
        list.positionViewAtBeginning()
        wait(50)
        const area = chip.mapToItem(window.contentItem, 0, 0)
        const clean = grabImage(window.contentItem)
        list.contentY = list.originY + 150
        wait(50)
        const scrolled = grabImage(window.contentItem)
        let covered = 0
        for (let x = area.x + 1; x < area.x + chip.width - 1; x += 3)
            for (let y = area.y + 1; y < area.y + chip.height - 1; y += 3)
                if (String(clean.pixel(x, y)) !== String(scrolled.pixel(x, y)))
                    covered++
        compare(covered, 0, "the filter is not covered by the scrolled list")
        // The filter in line with the notes.
        compare(findChild(page, "filter").rightPadding, findChild(page, "notesView").rightPadding)
        // End again after scrolling away: the last note comes back into view.
        list.forceActiveFocus()
        keyClick(Qt.Key_End)
        tryVerify(() => list.atYEnd, 1000)
        list.positionViewAtBeginning()
        wait(50)
        keyClick(Qt.Key_End)
        tryVerify(() => list.atYEnd, 1000, "End shows the last note again")
        // The scroll bar at the window's edge, not over the notes.
        const bar = list.parent.QQC2.ScrollBar.vertical
        if (bar) {
            const note = findChild(list.contentItem, "note")
            verify(note.mapToItem(page, note.width, 0).x <= bar.mapToItem(page, 0, 0).x + 1,
                   "note " + note.mapToItem(page, note.width, 0).x + ", bar " + bar.mapToItem(page, 0, 0).x)
        }
        // The keyboard reaches every note: Down moves through the list, which follows; Enter opens.
        list.positionViewAtBeginning()
        list.forceActiveFocus()
        list.currentIndex = 0
        for (let i = 0; i < 30; i++)
            keyClick(Qt.Key_Down)
        compare(list.currentIndex, 30)
        verify(list.contentY > 200, "the list follows the keyboard (" + list.contentY + ")")
        const target = page.shown[30].entry
        keyClick(Qt.Key_Return)
        tryVerify(() => window.pageStack.currentItem !== page, 2000)
        compare(window.pageStack.currentItem.entry.id, target)
        window.pageStack.pop()
        tryVerify(() => window.pageStack.currentItem === page)
        // Page Up up to the first note shows the count above it too.
        list.forceActiveFocus()
        keyClick(Qt.Key_End)
        tryVerify(() => list.atYEnd, 1000)
        for (let i = 0; i < 200 && list.currentIndex > 0; i++)
            keyClick(Qt.Key_PageUp)
        compare(list.currentIndex, 0)
        wait(500)
        verify(list.atYBeginning, "the count in view after Page Up (" + list.contentY + ", " + list.originY + ")")
        // Another filter starts at the top.
        list.positionViewAtEnd()
        page.filter = "aikido"
        tryVerify(() => list.atYBeginning, 1000, "another filter starts at the top")
        page.filter = ""
        // A remembered discipline that no longer exists falls back to all.
        page.filter = "kendo"
        tryCompare(page, "filter", "")
        // An empty filter says so (a discipline without notes, if one is left).
        const everything = page.shown
        const empty = Programme.disciplines.find(d => !everything.some(n => n.discipline === d.id))
        if (empty) {
            list.forceActiveFocus()
            list.currentIndex = 3
            page.filter = empty.id
            compare(findChild(page, "count").text, qsTr("No notes in this discipline."))
            tryCompare(list, "count", 0)
            compare(list.currentIndex, -1, "an empty list has no current note")
            keyClick(Qt.Key_Home)
            keyClick(Qt.Key_End)
            compare(list.currentIndex, -1, "an empty list has no current note")
            page.filter = ""
        }
        // The orphan is named as such.
        const orphan = everything.find(n => n.entry === "not-in-the-programme")
        compare(orphan.title, qsTr("Technique no longer in the programme"))
        // Notes → All notes → Notes → All notes does not pile up: the action goes back.
        const depth = window.pageStack.depth
        page.open(page.shown.find(n => n.kind === "technique" && n.entry !== "not-in-the-programme"))
        tryVerify(() => window.pageStack.depth === depth + 1)
        window.pageStack.currentItem.actions[0].trigger()
        tryVerify(() => window.pageStack.depth === depth, 2000, "back to the list instead of a new one")
        compare(window.pageStack.currentItem, page)
        // The filter is kept for the next time.
        page.filter = "daitoryu"
        window.pageStack.pop()
        const again = push("AllNotesPage")
        compare(again.filter, "daitoryu")
        again.filter = ""

        // A course notebook's entries join the list: their own filter, a tap opens the notebook,
        // the editor changes them through Courses.
        const course = Courses.create("Gastlehrer", "2026-05-01", "", "", "")
        verify(Courses.addEntry(course, "Irimi früher"))
        tryVerify(() => again.shown[0].kind === "course")
        compare(again.shown[0].title, "Gastlehrer")
        wait(800)
        mouseClick(findChild(again, "filter_courses"))
        tryVerify(() => again.filter === "courses")
        tryVerify(() => again.shown.length === 1)
        shot("8e-all-notes-courses")
        const entry = findChild(again, "note")
        mouseClick(entry, 20, entry.height - 10)
        tryVerify(() => window.pageStack.currentItem.uid === course, 2000)
        window.pageStack.pop()
        tryVerify(() => window.pageStack.currentItem === again)
        again.edit(again.shown[0])
        const courseEditor = findChild(again, "noteEditor")
        tryVerify(() => courseEditor.opened)
        verify(courseEditor.save(again.shown[0].uid, "Irimi noch früher"))
        courseEditor.close()
        tryVerify(() => again.shown[0].text === "Irimi noch früher")
        compare(Courses.entries(course)[0].text, "Irimi noch früher")
        verify(Courses.remove(course))
        tryVerify(() => again.shown.length === 0)
        compare(findChild(again, "count").text, qsTr("No entries in course notebooks yet."))
        again.filter = ""
        window.pageStack.pop()
    }

    // Exam mode: settings, a round with samples first (marked), rating at the end; "Apply" sets
    // the ratings and the statistics show the round.
    function test_9e_exam() {
        failOnWarning(/ReferenceError|TypeError/)
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k4")
        Exam.count = 8
        Exam.shuffled = false
        Exam.samples = true
        const tile = findChild(window.pageStack.items[0], "examTile")
        verify(tile.hint.indexOf("4. Kyu") >= 0, tile.hint)

        const page = push("ExamPage")
        compare(page.grade, "k4")
        compare(page.size, Exam.material("aikido", "k4").length)
        compare(page.counts[page.countIndex], 8)
        tryVerify(() => findChild(page, "samplesSwitch").enabled, 2000, "samples from 5. Kyu possible")
        verify(findChild(page, "grade_k5"), "a chip per grade")
        shot("11-exam")

        mouseClick(findChild(page, "startRound"))
        tryVerify(() => window.pageStack.currentItem !== page && window.pageStack.currentItem.points !== undefined)
        const round = window.pageStack.currentItem
        const n = round.points.length
        const samples = round.points.filter(p => p.sample).length
        verify(samples >= 2 && samples <= 5, "samples: " + samples)
        compare(n, 8 + samples)
        verify(round.points.slice(0, samples).every(p => p.sample && p.gradeName === "5. Kyu"), "samples first")
        const mark = findChild(round, "sampleMark")
        verify(mark.visible)
        verify(mark.text.indexOf("5. Kyu") >= 0, mark.text)
        compare(findChild(round, "counter").text, "1 / " + n)
        const board = findChild(round, "examBoard")
        compare(findChild(board, "boardAttack").visible, !!round.current.attack)
        verify(findChild(board, "boardTechnique").text.indexOf(round.current.title) >= 0)
        shot("12-exam-round")

        // Space and Enter move on; past the samples the mark goes.
        keyClick(Qt.Key_Space)
        compare(round.index, 1)
        keyClick(Qt.Key_Return)
        compare(round.index, 2)
        round.index = samples
        verify(!mark.visible, "own grade: no mark")
        const withAttack = round.points.findIndex(p => !p.sample && !!p.attack)
        if (withAttack >= 0) {
            round.index = withAttack
            verify(findChild(board, "boardAttack").visible)
            verify(findChild(board, "boardGroup").visible)
        }
        round.index = n - 1
        mouseClick(findChild(round, "nextButton"))
        tryVerify(() => window.pageStack.currentItem !== round && window.pageStack.currentItem.levels !== undefined)
        const end = window.pageStack.currentItem
        compare(window.pageStack.depth, 3, "the round page was replaced")

        // "Apply" only once every point is rated.
        const apply = findChild(end, "applyRound")
        wait(500)       // past the page transition, during which no button is enabled anyway
        verify(!apply.enabled)
        verify(!end.complete)
        const pickers = findChildren(end, "pointPicker").sort((a, b) => a.parent.index - b.parent.index)
        compare(pickers.length, n)
        let sum = 0
        for (let i = 0; i < n; ++i) {
            const level = i % 4 + 1
            sum += level
            pickers[i].picked(level)
            compare(end.complete, i === n - 1)
        }
        tryVerify(() => apply.enabled)
        compare(pickers[1].value, 2)
        shot("13-exam-end")
        apply.clicked()

        // The ratings are set, the statistics show the round (with the menu behind them).
        tryVerify(() => !!findChild(window.pageStack.currentItem, "examChart"))
        compare(window.pageStack.depth, 2)
        for (let i = 0; i < n; ++i) {
            compare(Practice.level(end.points[i].id), i % 4 + 1)
        }
        const stats = window.pageStack.currentItem
        compare(stats.discipline, "aikido")
        const chart = findChild(stats, "examChart")
        // Under Shinki the bars are muted ink, apart from the level colours of the legend above
        // (owner, 03.11.2026): an average of 2.3 does not read as "sure".
        if (uiShinki) {
            verify(Qt.colorEqual(chart.seriesColor(chart.series), Look.mutedTextColor), String(chart.seriesColor(chart.series)))
        }
        verify(chart.visible)
        compare(chart.values.length, 1)
        fuzzyCompare(chart.values[0], sum / n, 0.001)
        compare(chart.gridLines, 4)
        compare(chart.format(1), "1")
        compare(chart.format(4), "4")
        chart.hoverIndex = 0
        verify(chart.partNames[0].indexOf("4. Kyu") >= 0 && chart.partNames[0].indexOf(String(n)) >= 0, chart.partNames[0])
        wait(1000)
        stats.flickable.contentY = stats.flickable.contentHeight - stats.flickable.height
        shot("14-stats-exam")
        chart.hoverIndex = -1
        // Another discipline without rounds says so.
        stats.discipline = "daitoryu"
        verify(!chart.visible)
        verify(findChild(stats, "examRoundsText").text.length > 0)
        // A round with average 1 (all "seen") still shows a bar; the read-out says 1.
        verify(Practice.saveRound("daitoryu", "k5", [{ id: "exam-test-entry", level: 1 }]))
        verify(chart.visible)
        compare(chart.values.length, 1)
        compare(chart.values[0], 1)
        verify(chart.barHeight(0) >= KanteStyle.unit(3), "a bar at average 1: " + chart.barHeight(0))
        compare(chart.format(chart.values[0]), "1")
        stats.discipline = "aikido"
        window.pageStack.pop()

        // The settings show the last round.
        const again = push("ExamPage")
        verify(findChild(again, "examHint").text.indexOf(again.averageText(sum / n)) >= 0, findChild(again, "examHint").text)
        window.pageStack.pop()
    }

    // Shinki Toho: Shoden asks the Kamae as one point and Ô-dachi 1–11; the board shows only the
    // technique; no samples before the first grade. Cancel goes back to the settings.
    function test_9f_exam_shinkitoho() {
        failOnWarning(/ReferenceError|TypeError/)
        Programme.discipline = "shinkitoho"
        Programme.setGrade("shinkitoho", "shoden")
        Exam.count = 0
        Exam.samples = true
        const page = push("ExamPage")
        compare(page.size, 12)
        compare(page.counts, [0, 8])
        verify(!findChild(page, "samplesSwitch").enabled, "first grade")
        page.start()
        tryVerify(() => window.pageStack.currentItem !== page && window.pageStack.currentItem.points !== undefined)
        const round = window.pageStack.currentItem
        compare(round.points.length, 12)
        compare(round.points[0].id, "shinkitoho/kamae")
        verify(round.points.every(p => !p.sample))
        const board = findChild(round, "examBoard")
        verify(!findChild(board, "boardAttack").visible)
        verify(!findChild(board, "boardGroup").visible)
        verify(findChild(board, "boardTechnique").visible)
        verify(!findChild(round, "sampleMark").visible)
        round.index = 1
        compare(round.current.id, "shinkitoho/odachi/1")
        verify(!findChild(board, "boardAttack").visible)
        // Only number, name and kanji: the addition (In · Seigan → Hidari-gote) is the answer.
        verify(!!round.current.extra, "the form has an addition")
        verify(!findChild(board, "boardExtra").visible, "the board keeps the addition back")
        verify(findChild(board, "boardKanji").visible)
        shot("12b-exam-round-shinkitoho")
        // A tablet in landscape: the board grows.
        window.width = 1280
        window.height = 800
        tryVerify(() => round.tablet)
        compare(board.zoom, 1.6)
        shot("12c-exam-round-tablet")
        round.actions[0].trigger()
        tryVerify(() => window.pageStack.currentItem === page)
        window.pageStack.pop()
    }

    // Back from the rating asks once something is rated; without a rating it just goes back.
    function test_9g_exam_discard() {
        failOnWarning(/ReferenceError|TypeError/)
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        Exam.count = 8
        Exam.samples = false
        const settings = push("ExamPage")
        const depth = window.pageStack.depth
        settings.start()
        tryVerify(() => window.pageStack.currentItem.points !== undefined && window.pageStack.currentItem !== settings)
        const round = window.pageStack.currentItem
        round.index = round.points.length - 1
        round.next()
        tryVerify(() => window.pageStack.currentItem.levels !== undefined)
        let end = window.pageStack.currentItem
        const dialog = findChild(end, "discardDialog")
        verify(dialog)

        // Nothing rated: straight back.
        window.pageStack.goBack()
        tryVerify(() => window.pageStack.currentItem === settings)
        wait(300)       // the question would come a moment later (Qt.callLater)
        verify(window.pageStack.currentItem === settings, "stays back")
        verify(!dialog.opened)

        // Rated: the dialog asks, Cancel stays, Discard goes back without saving.
        settings.start()
        tryVerify(() => window.pageStack.currentItem.points !== undefined && window.pageStack.currentItem !== settings)
        window.pageStack.currentItem.index = window.pageStack.currentItem.points.length - 1
        window.pageStack.currentItem.next()
        tryVerify(() => window.pageStack.currentItem.levels !== undefined)
        end = window.pageStack.currentItem
        const asked = findChild(end, "discardDialog")
        const before = Practice.rounds("aikido", 8).length
        end.rate(0, 4)
        window.pageStack.goBack()
        tryVerify(() => asked.opened)
        verify(window.pageStack.currentItem === end, "still rating")
        shot("13b-exam-discard", true)
        // The text readable on the dialog's ground (it hangs on the overlay, outside the page's
        // scope; a dark platform scheme gave white on white), Cancel before the main button on
        // every platform.
        const question = findChild(asked.contentItem, "discardText")
        const ratio = contrast(question.color, asked.background.color)
        verify(ratio >= 4.5, question.color + " on " + asked.background.color + ": " + ratio.toFixed(1) + ":1")
        const cancel = asked.standardButton(QQC2.Dialog.Cancel)
        const discard = asked.standardButton(QQC2.Dialog.Ok)
        verify(cancel.mapToItem(null, 0, 0).x < discard.mapToItem(null, 0, 0).x, "Cancel first")
        asked.reject()
        tryVerify(() => !asked.opened)
        verify(window.pageStack.currentItem === end, "still rating")
        window.pageStack.goBack()
        tryVerify(() => asked.opened)
        asked.accept()
        tryVerify(() => window.pageStack.currentItem === settings)
        compare(window.pageStack.depth, depth)
        compare(Practice.rounds("aikido", 8).length, before)
        window.pageStack.pop()
    }

    // The Kamae player: its 26 positions come last, so "Done" and Back stay where they are for a
    // technique without positions (in view without scrolling, also sideways), and a large
    // tablet keeps the picture as large.
    function test_9y_kamae_player() {
        failOnWarning(/ReferenceError|TypeError/)
        const kamae = Programme.entry("shinkitoho/kamae")
        const suburi = Programme.entry("shinkitoho/suburi/1")
        if (!kamae.video || !Videos.available(kamae.video.video)) {
            skip("no video for the Kamae exercise (set MITORI_VIDEOS)")
        }
        verify(Practice.rate(kamae.id, 2))      // rated today: the training is done at once
        verify(Practice.rate(suburi.id, 2))
        // [width, height, training, the item that must be in view]
        for (const c of [[420, 800, true, "trainingDone"], [915, 412, false, "backButton"], [915, 412, true, "trainingDone"]]) {
            window.width = c[0]
            window.height = c[1]
            const at = entry => {
                const page = push("PlayerPage", { playlist: [entry], index: 0, training: c[2] })
                tryVerify(() => page.trainingDone === c[2])
                wait(500)
                const view = findChild(page, "controlsView")
                compare(view.contentY, 0)
                const item = findChild(page, c[3])
                verify(item.visible, c[3])
                const y = item.mapToItem(view, 0, 0).y
                const result = { y: y, bottom: y + item.height, height: view.height, steps: findChild(page, "stepList").count }
                window.pageStack.pop()
                return result
            }
            const k = at(kamae)
            const s = at(suburi)
            compare(k.steps, 26)
            compare(s.steps, 0)
            verify(Math.abs(k.y - s.y) < 1, c[3] + " at " + c + ": " + k.y + " with the positions, " + s.y + " without")
            // Done and Back stand first: in view upright and sideways.
            verify(k.bottom <= k.height, c[3] + " in view at " + c + " (" + k.bottom + " of " + k.height + ")")
        }
        // A large tablet beside the list: the picture as large as for a technique without positions.
        Programme.discipline = "shinkitoho"
        Programme.setGrade("shinkitoho", "shoden")
        window.width = 1920
        window.height = 1080
        const programme = push("ProgrammePage")
        tryVerify(() => programme.split)
        const pane = findChild(programme, "playerPane")
        programme.open(programme.rows.find(r => r.id === suburi.id))
        tryCompare(pane.item.current, "id", suburi.id, 3000)
        const stage = findChild(pane.item, "stage")
        wait(300)
        const without = stage.height
        verify(without > stage.width / stage.ratio / 2 + 1, "more than half the picture: " + without)
        programme.open(programme.rows.find(r => r.id === kamae.id))
        tryCompare(pane.item.current, "id", kamae.id, 3000)
        tryCompare(findChild(pane.item, "stepList"), "count", 26)
        wait(300)       // laid out with the positions
        verify(Math.abs(stage.height - without) < 1, "picture " + stage.width + "×" + stage.height + ", without positions " + without)
        window.width = 420
        window.height = 800
        window.pageStack.pop()
        Programme.discipline = "aikido"
    }

    function test_9z_stats() {
        // A rating today makes today a practice day (Shinki Toho: the other cases leave it alone;
        // the cases run in name order, this one last).
        const grade = Programme.grades("shinkitoho")[0].id
        const rows = Programme.entries("shinkitoho", grade).filter(r => r.kind === "entry")
        verify(Practice.rate(rows[0].id, 2))
        const page = push("StatsPage")
        compare(page.last30.days, 1)
        compare(findChild(page, "practiceDays").value, "1")
        // The open page follows new ratings; a technique counts once a day.
        const before = page.last30.techniques
        verify(Practice.rate(rows[0].id, 3))
        verify(Practice.rate(rows[1].id, 1))
        verify(Practice.rate(rows[1].id, 2))
        compare(page.last30.techniques, before + 1)
        compare(findChild(page, "techniquesPractised").value, String(before + 1))
        const heat = findChild(page, "practiceHeatmap")
        verify(heat.weeks >= 4 && heat.weeks <= 26)
        const today = heat.isoDay(new Date())
        verify(heat.levels[today] >= 1, "today coloured")
        verify(heat.tipOf(new Date()).indexOf(String(page.perDay[today])) >= 0, heat.tipOf(new Date()))
        // A day without practice says so.
        const yesterday = new Date()
        yesterday.setDate(yesterday.getDate() - 1)
        verify(heat.tipOf(yesterday).indexOf(qsTr("no practice")) > 0, heat.tipOf(yesterday))
        // Each grade shows its figures without hovering: exam-ready of all.
        const stands = findChildren(page, "statsGradeStand")
        verify(stands.length > 3, "a bar per grade")
        const figures = findChildren(page, "gradeFigure")
        verify(figures.length > 3)
        verify(/\d+.*\d+/.test(figures[0].text), figures[0].text)
        shot("10-stats")
        // My grades of the discipline.
        verify(Courses.addGrade("daitoryu", "k5", "2025-05-01", "Bonn"))
        page.discipline = "daitoryu"
        tryVerify(() => findChildren(page, "statsGrade").length === 1)
        verify(findChild(page, "editGrades").visible)
        window.pageStack.pop()

        // All years: this year's calendar first, with the day; one discipline only.
        const years = push("YearsPage")
        const year = new Date().getFullYear()
        const block = findChild(years, "year_" + year)
        verify(block, "this year")
        verify(findChild(block, "yearHeading").text.indexOf(qsTr("%n practice day(s)", "", 1)) > 0,
               findChild(block, "yearHeading").text)
        // Every day says what it was, also one without practice.
        const dayBefore = new Date()
        dayBefore.setDate(dayBefore.getDate() - 1)
        const yearHeat = findChild(block, "yearHeatmap")
        if (dayBefore.getFullYear() === year) {
            verify(yearHeat.tipOf(dayBefore).indexOf(qsTr("no practice")) > 0, yearHeat.tipOf(dayBefore))
        }
        years.discipline = "shinkitoho"
        verify(findChild(findChild(years, "year_" + year), "yearHeading").text.indexOf(qsTr("%n practice day(s)", "", 1)) > 0)
        shot("10b-years")
        window.pageStack.pop()
        Courses.removeGrade(Courses.grades.find(g => g.discipline === "daitoryu").uid)
        // The start tile counts the practice days.
        const tile = findChild(window.pageStack.items[0], "statsTile")
        compare(tile.days, 1)
    }

    // Tablet in landscape: list and small player on one page; a row with a video plays beside the
    // list, the player moving on marks its row.
    function test_2t_tablet() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry" && r.video)
        if (rows.length < 2 || !Videos.available(rows[0].video.video)) {
            skip("no video for Aikido 5. Kyu (set MITORI_VIDEOS)")
        }
        window.width = 1280
        window.height = 800
        const page = push("ProgrammePage")
        tryVerify(() => page.split, 2000, "list and player side by side")
        const pane = findChild(page, "playerPane")
        tryVerify(() => pane.item && pane.item.playing, 10000)
        compare(pane.item.compact, true)
        // A row with a video plays beside the list, the page stays.
        page.open(rows[1])
        tryCompare(pane.item.current, "id", rows[1].id, 3000)
        compare(window.pageStack.currentItem, page)
        compare(page.lastId, rows[1].id)
        // The player moves on: its row is marked, and the list shows it.
        pane.item.go(1)
        tryCompare(page, "lastId", page.playlist[pane.item.index].id)
        const list = findChild(page, "programmeList")
        pane.item.index = page.playlist.length - 1
        tryCompare(page, "lastId", page.playlist[page.playlist.length - 1].id)
        const row = page.rows.findIndex(r => r.id === page.lastId)
        tryVerify(() => {
            const first = list.indexAt(10, list.contentY + 5), last = list.indexAt(10, list.contentY + list.height - 5)
            return first >= 0 && first <= row && (last < 0 || last >= row)
        }, 2000, "the list scrolled to the playing row")
        shot("2t-tablet")
        // Turned to portrait and back: the chosen technique stays.
        page.open(page.playlist[3])
        tryCompare(pane.item.current, "id", page.playlist[3].id, 3000)
        const chosen = page.playlist[3].id
        window.width = 800
        window.height = 1280
        tryVerify(() => !page.split)
        window.width = 1280
        window.height = 800
        tryVerify(() => pane.item && pane.item.current.id === chosen, 3000, "still " + chosen)
        compare(page.lastId, chosen)
        // Another grade: its first technique plays, not the same place in another list.
        Programme.setGrade("aikido", "k4")
        page.grade = "k4"
        tryVerify(() => pane.item && pane.item.current.id === page.playlist[0].id, 3000)
        compare(page.lastId, page.playlist[0].id)
        // A phone in landscape is no tablet: no split, ▶ opens the player page.
        window.width = 915
        window.height = 412
        tryVerify(() => !page.split, 2000, "phone landscape")
        window.width = 420
        window.height = 800
        tryVerify(() => !page.split)
        Programme.setGrade("aikido", "k5")
        window.pageStack.pop()
    }

    // Tablet: a technique with a comment (Shinki Toho) shows the way to it beside the player.
    function test_2u_tablet_comment() {
        const grade = Programme.grades("shinkitoho")[0].id
        const rows = Programme.entries("shinkitoho", grade).filter(r => r.kind === "entry" && r.video && r.comment)
        if (rows.length === 0 || !Videos.available(rows[0].video.video)) {
            skip("no video for Shinki Toho (set MITORI_VIDEOS)")
        }
        Programme.discipline = "shinkitoho"
        Programme.setGrade("shinkitoho", grade)
        window.width = 1280
        window.height = 800
        const page = push("ProgrammePage")
        tryVerify(() => page.split)
        page.open(rows[0])
        const pane = findChild(page, "playerPane")
        tryCompare(pane.item.current, "id", rows[0].id, 3000)
        const button = findChild(pane.item, "commentButton")
        verify(button && button.visible, "comment beside the player")
        button.clicked()
        tryVerify(() => window.pageStack.currentItem !== page, 2000)
        compare(window.pageStack.currentItem.entry.id, rows[0].id)
        tryVerify(() => !pane.item.playing, 2000, "the player pauses under another page")
        // Turned under the comment: the player made anew stays quiet.
        window.width = 800
        window.height = 1280
        tryVerify(() => !page.split)
        window.width = 1280
        window.height = 800
        tryVerify(() => pane.item && pane.item.duration > 0, 10000)
        wait(500)
        verify(!pane.item.playing, "no sound under the comment page")
        window.pageStack.pop()
        window.width = 420
        window.height = 800
        window.pageStack.pop()
        Programme.discipline = "aikido"
    }

    // Tablet: the Kamae exercise plays beside the list, with its 26 positions under the player.
    function test_2u_tablet_kamae() {
        failOnWarning(/ReferenceError|TypeError/)
        const kamae = Programme.entry("shinkitoho/kamae")
        if (!kamae.video || !Videos.available(kamae.video.video)) {
            skip("no video for the Kamae exercise (set MITORI_VIDEOS)")
        }
        Programme.discipline = "shinkitoho"
        Programme.setGrade("shinkitoho", "shoden")
        window.width = 1280
        window.height = 800
        const page = push("ProgrammePage")
        tryVerify(() => page.split)
        page.open(page.rows.find(r => r.id === "shinkitoho/kamae"))
        const pane = findChild(page, "playerPane")
        tryCompare(pane.item.current, "id", "shinkitoho/kamae", 3000)
        const list = findChild(pane.item, "stepList")
        verify(list && list.visible, "the positions beside the player")
        tryCompare(list, "count", 26)
        shot("2u-tablet-kamae")
        // A form has none.
        page.open(page.playlist.find(r => r.id.startsWith("shinkitoho/odachi/")))
        tryVerify(() => pane.item.current.id.startsWith("shinkitoho/odachi/"), 3000)
        verify(!findChild(pane.item, "stepList").visible)
        window.width = 420
        window.height = 800
        window.pageStack.pop()
        Programme.discipline = "aikido"
    }

    // Tablet in landscape: the start menu as a grid of four; the fifth tile starts a second row.
    function test_1b_start_tablet() {
        const start = window.pageStack.items[0]
        window.width = 1280
        window.height = 800
        const tiles = findChild(start, "tiles")
        tryCompare(tiles, "columns", 4)
        const shown = tiles.children.filter(t => t.visible && t.width > 0)
        verify(shown.length > 4, "a second row")
        // Four to a row, in order: the fifth starts the next row under the first.
        shown.forEach((t, i) => {
            const first = shown[i - i % 4]
            verify(Math.abs(t.y - first.y) < 1, "tile " + i + " in its row")
            if (i % 4)
                verify(t.x > shown[i - 1].x + shown[i - 1].width - 1, "tile " + i + " right of the one before")
            else if (i)
                verify(Math.abs(t.x - shown[0].x) < 1 && t.y > shown[i - 4].y, "tile " + i + " starts a row")
        })
        shot("1b-start-tablet")
        window.width = 420
        window.height = 800
        tryCompare(tiles, "columns", 1)
    }

    // The keyboard scrolls the programme list (Down, Page Down).
    function test_2v_programme_keys() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k3")
        const page = push("ProgrammePage")
        const list = findChild(page, "programmeList")
        wait(500)
        page.forceActiveFocus()
        const before = list.contentY
        keyClick(Qt.Key_PageDown)
        tryVerify(() => list.contentY > before + 100, 2000, "Page Down")
        const after = list.contentY
        keyClick(Qt.Key_Down)
        tryVerify(() => list.contentY > after, 2000, "Down")
        wait(300)
        compare(Math.round(list.contentY - after), Kirigami.Units.gridUnit * 2, "Down scrolls two lines")
        keyClick(Qt.Key_Home)
        tryVerify(() => list.contentY <= list.originY + 1, 2000, "Home")
        // Also with a row in focus (after a click or Tab): the list scrolls, not a current index.
        let i = 0
        while (i < list.count && !(list.itemAtIndex(i) && list.itemAtIndex(i).item && list.itemAtIndex(i).item.entry))
            i++
        verify(i < list.count, "a technique row in view")
        list.itemAtIndex(i).item.forceActiveFocus()
        const y = list.contentY
        keyClick(Qt.Key_Down)
        tryVerify(() => list.contentY > y, 2000, "Down on a focused row")
        wait(300)
        compare(Math.round(list.contentY - y), Kirigami.Units.gridUnit * 2, "Down on a row scrolls two lines")
        // Tab goes from row to row down the whole list, each in view (a virtual list: the rows
        // below are made as it scrolls).
        list.positionViewAtBeginning()
        wait(100)
        let first = 0
        while (!(list.itemAtIndex(first) && list.itemAtIndex(first).item && list.itemAtIndex(first).item.entry))
            first++
        list.itemAtIndex(first).item.forceActiveFocus(Qt.TabFocusReason)
        const rowOf = item => {
            for (let p = item; p; p = p.parent)
                if (p.entry !== undefined && p.parent && p.parent.modelData !== undefined)
                    return p
            return null
        }
        const tabbable = page.rows.filter(r => r.kind === "entry" || r.kind === "info").map(r => r.id)
        let seen = [list.itemAtIndex(first).item.entry.id]
        for (let n = 0; n < tabbable.length + 5; n++) {
            keyClick(Qt.Key_Tab)
            wait(20)
            const row = rowOf(window.activeFocusItem)
            if (!row)
                break                                   // past the last row: out of the list
            const top = row.mapToItem(list, 0, 0).y
            verify(top >= -1 && top + row.height <= list.height + 1, "the row in view after Tab " + (n + 1) + " (" + top + ")")
            seen.push(row.entry.id)
        }
        compare(seen, tabbable, "Tab reaches every row once, in order, then leaves the list")
        // Shift+Tab from the last row goes back up.
        list.positionViewAtEnd()
        wait(100)
        const lastRow = list.itemAtIndex(page.rows.findIndex(r => r.id === tabbable[tabbable.length - 1])).item
        lastRow.forceActiveFocus(Qt.TabFocusReason)
        keyClick(Qt.Key_Backtab)
        compare(rowOf(window.activeFocusItem).entry.id, tabbable[tabbable.length - 2])
        // End reaches the end at the first press (rows of different heights: the list estimates
        // its height until it made the last ones; it stopped 6 px short).
        keyClick(Qt.Key_End)
        wait(300)
        verify(list.atYEnd, "End at the first press (" + list.contentY + ")")
        Programme.setGrade("aikido", "k5")
        window.pageStack.pop()
    }

    // Tablet: keys in the small player stay there (End does not scroll the list), and on a small
    // tablet the player's controls fit under the picture.
    function test_2w_tablet_player_keys() {
        Programme.discipline = "aikido"
        Programme.setGrade("aikido", "k5")
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry" && r.video)
        if (rows.length < 2 || !Videos.available(rows[0].video.video)) {
            skip("no video for Aikido 5. Kyu (set MITORI_VIDEOS)")
        }
        window.width = 1024
        window.height = 560
        const page = push("ProgrammePage")
        tryVerify(() => page.split, 2000, "a small tablet splits too")
        const pane = findChild(page, "playerPane")
        tryVerify(() => pane.item && pane.item.duration > 0, 10000)
        const list = findChild(page, "programmeList")
        pane.item.forceActiveFocus()
        const y = list.contentY
        keyClick(Qt.Key_End)
        keyClick(Qt.Key_PageDown)
        wait(100)
        compare(list.contentY, y, "keys in the player leave the list alone")
        const controls = findChild(pane.item, "controlsView")
        const bar = findChild(pane.item, "transportBar")
        const bottom = bar.mapToItem(controls, 0, bar.height).y
        verify(bottom <= controls.height, "the transport bar in view (" + bottom + " of " + controls.height + ")")
        // Tab through the controls: the focused one stays in view.
        bar.nextItemInFocusChain(true).forceActiveFocus(Qt.TabFocusReason)
        verify(controls.contentHeight > controls.height, "the controls scroll on a small tablet")
        for (let n = 0; n < 25; n++) {
            keyClick(Qt.Key_Tab)
            const f = window.activeFocusItem
            let inside = false
            for (let p = f; p; p = p.parent)
                inside = inside || p === controls
            if (!inside)
                break
            const top = f.mapToItem(controls, 0, 0).y
            verify(top >= -1 && top + Math.min(f.height, controls.height) <= controls.height + 1,
                   "focus in view after " + (n + 1) + " Tab (" + top + ", " + f.height + " of " + controls.height + ")")
        }
        // A large tablet keeps the whole picture: only as little height as the controls need.
        window.width = 1920
        window.height = 1080
        tryVerify(() => pane.item && pane.item.height > 900, 2000)
        const stage = findChild(pane.item, "stage")
        const controls2 = findChild(pane.item, "controlsView")
        tryVerify(() => Math.abs(stage.height - stage.width / stage.ratio) < 1
                         || Math.abs(controls2.contentHeight - controls2.height) < 1, 2000,
                  "picture " + stage.width + "×" + stage.height + ", controls " + controls2.contentHeight + " of " + controls2.height)
        verify(controls2.contentHeight <= controls2.height + 1, "the controls fit")
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

    // Photos and voice notes: a picked picture becomes a note with the text typed so far, shown
    // in the notes, the notebook and All notes; a voice note plays; the camera page says when
    // there is no camera (offscreen). Deleting a note deletes its file.
    MediaDevices {
        id: devices
    }

    function test_8f_photo_and_voice() {
        failOnWarning(/ReferenceError|TypeError|Binding loop/)
        const entry = Programme.entries("aikido", "k4").find(r => r.kind === "entry")
        const page = push("NotesPage", { entry: entry })
        const compose = findChild(page, "compose")
        compose.text = "Fußstellung"
        const name = Media.importPhoto("file://" + testMediaDir + "/foto.png")
        verify(name.endsWith(".jpg"), name)
        const media = findChild(page.footer, "noteMedia")
        media.attached(name)
        tryVerify(() => Practice.notes(entry.id).some(n => n.media === name))
        // A file that is no picture says so (and makes no note).
        const before = Practice.notes(entry.id).length
        media.importPicture("file://" + testMediaDir + "/README.md")
        tryVerify(() => media.problem.shown, 2000, "a message for an unreadable picture")
        compare(Practice.notes(entry.id).length, before)
        compare(compose.text, "", "the text went with the photo")
        tryVerify(() => findChildren(page, "notePhoto").some(i => i.visible && i.status === Image.Ready), 3000)
        shot("8f-note-photo")

        // A voice note without text: named as such, it plays.
        verify(Practice.addNote(entry.id, "", "test-stimme.m4a"))
        tryVerify(() => findChildren(page, "note").some(n => n.text === qsTr("Voice note")))
        const play = findChildren(page, "playVoice").find(b => b.visible)
        verify(play)
        wait(800)
        mouseClick(play)
        // Loaded, it shows its length ("Voice note 0:02"); playing, the button stops it.
        tryVerify(() => findChildren(page, "voiceLength").some(l => l.visible && /\d:\d\d$/.test(l.text)), 5000,
                  findChildren(page, "voiceLength").map(l => l.text).join(", "))

        // Without a camera the camera page says so (a machine with a webcam does not switch it on
        // for a test).
        if (devices.videoInputs.length === 0) {
            const camera = push("CameraPage")
            tryVerify(() => findChild(camera, "cameraError").visible, 3000)
            verify(!findChild(camera, "shutter").enabled)
            window.pageStack.pop()
        }

        // A file that is gone (a cloud backup leaves the media out) says so instead of an empty picture.
        verify(Practice.addNote(entry.id, "", "fehlt-" + name))
        tryVerify(() => findChildren(page, "mediaMissing").some(l => l.visible && l.text === qsTr("The photo is not on this device.")),
                  3000, "a missing photo is named")
        verify(Practice.deleteNote(Practice.notes(entry.id).find(n => n.media === "fehlt-" + name).uid))

        // All notes shows the photo too.
        const all = push("AllNotesPage")
        all.filter = ""
        tryVerify(() => findChildren(all, "notePhoto").some(i => i.visible))
        window.pageStack.pop()

        // Deleted, the photo's file goes.
        const photoNote = Practice.notes(entry.id).find(n => n.media === name)
        verify(Practice.deleteNote(photoNote.uid))
        const voiceNote = Practice.notes(entry.id).find(n => n.media === "test-stimme.m4a")
        verify(Practice.changeNote(voiceNote.uid, ""), "a voice note needs no text")
        window.pageStack.pop()

        // The sheet's "last note" names a voice note without text as such.
        Programme.discipline = "aikido"         // earlier cases leave another one chosen
        Programme.setGrade("aikido", "k4")
        const programme = push("ProgrammePage")
        programme.open(programme.rows.find(r => r.id === entry.id))
        const sheet = findChild(programme, "techniqueSheet")
        tryVerify(() => sheet.opened)
        const last = findChild(sheet.contentItem, "sheetLastNote")
        verify(last.visible, "a voice note is a last note too")
        verify(last.text.endsWith(": " + qsTr("Voice note")), last.text)
        sheet.close()
        tryVerify(() => !sheet.opened)
        window.pageStack.pop()

        // A notebook entry with a photo.
        const course = Courses.create("Fotolehrgang", "2026-05-01", "", "", "")
        const book = push("CoursePage", { uid: course })
        findChild(book, "courseCompose").text = "Tafel"
        findChild(book, "courseMedia").attached(Media.importPhoto("file://" + testMediaDir + "/foto.png"))
        tryVerify(() => Courses.entries(course).length === 1 && Courses.entries(course)[0].media.endsWith(".jpg"))
        tryVerify(() => findChildren(book, "notePhoto").some(i => i.visible))
        // Exported as a ZIP (the file dialog is the platform's; the page's step after it).
        const target = "file://" + uiShotsDir + "/" + Courses.exportName(course)
        book.exportTo(target)
        tryVerify(() => findChild(book, "exportedTo").visible)
        verify(findChild(book, "exportedTo").text.indexOf("Fotolehrgang") > 0, findChild(book, "exportedTo").text)
        shot("14d-course-exported")
        window.pageStack.pop()
        verify(Courses.remove(course))
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
        const page = playerAt(0)                       // skips without videos, before the window changes
        window.width = 800
        window.height = 360
        wait(300)
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
