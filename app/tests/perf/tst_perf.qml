import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import Mitori

// How long the pages take with five years of use (tst_perf.cpp). "PERF <name> <ms> cpu <ms>" for the
// record; a budget that fails means: much slower than it was (generous, the CI's machine is small).
// Budgets hold CPU time: the time on the wall also counts who else runs (Player 5597 ms with 40 busy
// processes, 1312 ms CPU), and says nothing about the page.
TestCase {
    id: test

    name: "Perf"
    when: windowShown

    readonly property int pageBudget: 5000
    readonly property int callBudget: 2000
    property var window: null
    property var times: []

    function initTestCase() {
        const component = Qt.createComponent("Mitori", "Main")
        verify(component.status === Component.Ready, component.errorString())
        const started = now()
        window = component.createObject(null, { width: 420, height: 800 })
        window.show()
        tryVerify(() => window.pageStack.currentItem !== null, 10000)
        waitForRendering(window.contentItem)
        report("start menu", started, 8000)
    }

    function cleanupTestCase() {
        console.log("PERF slowest " + times.sort((a, b) => b.ms - a.ms).slice(0, 5).map(t => t.name + " " + t.ms).join(", "))
        window.destroy()
    }

    // Kirigami 6.31 crashes on a pop in the last moments of a slide: ColumnView::setCurrentIndex
    // hands the focus to the page below, a Behavior on that makes Qt run the slide's end there and
    // then, which takes the current item away (SIGSEGV in QQuickItem::size). A tap does not get
    // here, a page does not take taps while it slides; a test does.
    function popPage() {
        tryVerify(() => !window.pageStack.columnView.moving, 5000, "the slide ends")
        // A page may have gone during the wait (TrainingEndPage): the start menu stays.
        if (window.pageStack.depth > 1) {
            window.pageStack.pop()
        }
    }

    // A failed budget must not leave its page in the stack (it would slow what comes next).
    function cleanup() {
        while (window.pageStack.depth > 1) {
            popPage()
        }
        wait(50)
    }

    function now() {
        return { wall: Date.now(), cpu: perfClock.cpuMs() }
    }

    // Since `started`; `waited`: what the case itself waits for, not part of the wall's time.
    function report(name, started, budget, waited) {
        const wall = Date.now() - started.wall - (waited || 0)
        const ms = perfClock.cpuMs() - started.cpu
        console.log("PERF " + name + " " + wall + " cpu " + ms)
        times.push({ name: name, ms: ms })
        verify(ms <= budget, name + " took " + ms + " ms CPU, budget " + budget)
    }

    // A page from the push until it is drawn.
    function page(name, file, properties) {
        const started = now()
        window.pageStack.push(Qt.resolvedUrl("qrc:/qt/qml/Mitori/qml/" + file + ".qml"), properties || {})
        waitForRendering(window.contentItem)
        report(name, started, pageBudget)
        const shown = window.pageStack.currentItem
        wait(350)   // the slide, not part of the time
        return shown
    }

    function back() {
        popPage()
        wait(50)
    }

    function call(name, f) {
        const started = now()
        const result = f()
        report(name, started, callBudget)
        return result
    }

    function test_1_pages() {
        for (const name of ["TodayPage", "StatsPage", "YearsPage", "AllNotesPage", "TechniqueMapPage", "SearchPage",
                            "CoursesPage", "NotebooksPage", "GradesPage", "SettingsPage", "BackupPage", "VideosPage", "AppearancePage", "ExamPage"]) {
            page(name, name)
            back()
        }
    }

    function test_2_programmes() {
        for (const discipline of ["aikido", "daitoryu", "shinkitoho"]) {
            Programme.discipline = discipline
            page("Programme " + discipline, "ProgrammePage")
            back()
        }
        Programme.discipline = "aikido"
    }

    // Beside the list on a tablet the page has the small player too.
    function test_3_tablet() {
        window.width = 1280
        window.height = 800
        wait(200)
        page("Programme tablet", "ProgrammePage")
        back()
        window.width = 420
        window.height = 800
        wait(200)
    }

    function test_4_player() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        page("Player", "PlayerPage", { playlist: rows, index: 0 })
        back()
    }

    function test_5_calls() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        call("Programme.entries", () => Programme.entries("aikido", "k5"))
        call("Practice.due", () => Practice.due(rows))
        call("Practice.counts", () => Practice.counts(rows.map(r => r.id)))
        call("Practice.allNotes", () => Practice.allNotes())
        call("Practice.searchNotes", () => Practice.searchNotes("Hüfte"))
        call("Practice.practisedPerDay", () => Practice.practisedPerDay(84))
        call("Practice.practisedInYear", () => Practice.practisedInYear(new Date().getFullYear(), "aikido"))
    }

    // A long note in the technique picker: it asks on the UI thread.
    function test_5b_suggest_long_note() {
        const sentence = "Heute Ikkyo aus Katate-dori geübt, dann die Hüfte tief. Danach Kote-gaeshi, langsam und sauber! "
        const words = sentence.trim().split(" ").length
        const text = sentence.repeat(Math.ceil(500 / words))
        call("Programme.suggest, 500 words", () => Programme.suggest(text))
    }

    // One keystroke in the search: the list follows at once.
    function test_6_search_typing() {
        const search = page("Search", "SearchPage")
        const field = findChild(search, "searchField")
        for (const text of ["k", "ko", "kot", "kote"]) {
            const started = now()
            field.text = text
            wait(320)   // the page waits for the typing to pause
            wait(0)     // the list has followed (no waitForRendering: an unchanged picture never comes)
            report("Search “" + text + "”", started, callBudget, 320)
        }
        back()
    }

    // The save that everyone does: a rating with the list and the player open (Practice.changed).
    function test_7_rate() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        page("Programme", "ProgrammePage")
        page("Player", "PlayerPage", { playlist: rows, index: 0 })
        call("Practice.rate", () => Practice.rate(rows[0].id, 3))
        call("Practice.addNote", () => Practice.addNote(rows[0].id, "Noch einmal"))
    }

    // The same with the statistics: the pages that count everything, under the player.
    function test_8_rate_over_stats() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        page("Stats", "StatsPage")
        page("Years", "YearsPage")
        page("Today", "TodayPage")
        page("Player", "PlayerPage", { playlist: rows, index: 0 })
        call("Practice.rate under stats", () => Practice.rate(rows[0].id, 2))
    }
}
