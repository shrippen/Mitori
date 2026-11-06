#include "sync.h"

#include "coursebook.h"
#include "courses.h"
#include "device.h"
#include "mediastore.h"
#include "practice.h"
#include "practicelog.h"

#include <QCoreApplication>
#include <QDir>
#include <QSysInfo>
#include <QtConcurrent/QtConcurrentRun>

namespace {
const QString kNotes = QStringLiteral("notes");

// The row's text now: changed again after the sync, that change is "mine" (not the older text).
template<typename Rows>
QString currentText(const Rows &rows, const TableMerge::Conflict &c)
{
    for (const auto &row : rows) {
        if (row.uid == c.uid) {
            return row.text;
        }
    }
    return c.mine;
}

const QString kEntries = QStringLiteral("entries");
const QString kLocalHost = QStringLiteral("localhost");
}

Sync::Sync(QObject *parent)
    : Sync(Practice::defaultPath(), Courses::defaultPath(), MediaStore::defaultRoot(), SyncAccount::defaultFolder(),
           parent)
{
    // After the start (the app is up first), then once a day while it runs.
    m_daily.setInterval(kDailyMs);
    connect(&m_daily, &QTimer::timeout, this, &Sync::syncNow);
    if (enabled()) {
        m_soon.start(kStartDelayMs);
        m_daily.start();
    }
}

Sync::Sync(const QString &practice, const QString &courses, const QString &media, const QString &folder,
           QObject *parent)
    : QObject(parent)
    , m_practice(practice)
    , m_courses(courses)
    , m_media(media)
    , m_work(QDir(folder).filePath(QStringLiteral("arbeit")))
    , m_account(folder)
{
    m_soon.setSingleShot(true);
    connect(&m_soon, &QTimer::timeout, this, &Sync::syncNow);
    connect(&m_watcher, &QFutureWatcher<SyncJob::Output>::finished, this, &Sync::finished);
}

Sync::~Sync()
{
    m_watcher.waitForFinished();
}

void Sync::setEnabled(bool on)
{
    if (on == enabled()) {
        return;
    }
    m_account.setEnabled(on ? SyncAccount::Toggle::On : SyncAccount::Toggle::Off);
    if (on) {
        m_account.setReplaced(SyncAccount::Toggle::Off);
        m_daily.start();
    } else {
        m_daily.stop();
        m_soon.stop();
    }
    emit accountChanged();
}

void Sync::setAutomatic(bool on)
{
    if (on == automatic()) {
        return;
    }
    m_account.setAutomatic(on ? SyncAccount::Toggle::On : SyncAccount::Toggle::Off);
    emit accountChanged();
}

// The name the user gave, else the computer's (a phone's host name says nothing: its system).
QString Sync::deviceName() const
{
    const QString name = m_account.name();
    if (!name.isEmpty()) {
        return name;
    }
    const QString host = QSysInfo::machineHostName();
    return host.isEmpty() || host == kLocalHost ? QSysInfo::prettyProductName() : host;
}

void Sync::setAccount(const QString &server, const QString &user, const QString &password, const QString &name)
{
    WebDav::Account a = m_account.account();
    QString folder = server.trimmed();
    if (!folder.isEmpty() && !folder.endsWith(QLatin1Char('/'))) {
        folder += QLatin1Char('/');
    }
    a.folder = QUrl(folder);
    a.user = user.trimmed();
    if (!password.isEmpty()) {
        a.password = password;
    }
    m_account.setAccount(a);
    m_account.setName(name);
    emit accountChanged();
}

bool Sync::syncNow()
{
    // One runs: another follows it (a change after its snapshot would wait for the next day).
    if (m_busy) {
        m_again = true;
        return true;
    }
    const WebDav::Account account = m_account.account();
    if (!enabled() || !account.folder.isValid() || account.folder.isEmpty() || account.user.isEmpty()) {
        setError(tr("Enter the server, user and app password first."));
        return false;
    }
    const SyncJob::Input input{account,  m_account.id(), deviceName(), deviceId(),     m_account.base(),
                               m_account.conflicts(), m_practice, m_courses, m_media, m_work};
    m_busy = true;
    emit busyChanged();
    m_watcher.setFuture(QtConcurrent::run(&SyncJob::run, input));
    return true;
}

void Sync::touched()
{
    if (enabled() && automatic() && !m_merging) {
        m_soon.start(kAfterChangeMs);
    }
}

void Sync::wait()
{
    m_watcher.waitForFinished();
    finished();
}

void Sync::finished()
{
    if (!m_busy) {
        return;
    }
    m_busy = false;
    if (m_again) {
        m_again = false;
        m_soon.start(kAfterChangeMs);
    }
    const SyncJob::Output out = m_watcher.result();
    if (out.ok) {
        m_account.setLast(out.started);
        m_account.setDevices(out.devices);
        m_account.setConflicts(out.conflicts);
        m_account.setBase(out.base);
        m_error.clear();
    } else {
        m_error = tr("The sync failed: %1").arg(out.error);
    }
    emit busyChanged();
    emit stateChanged();
    if (out.received > 0 || !out.conflicts.isEmpty()) {
        announce();
    }
}

void Sync::announce()
{
    m_merging = true;
    emit merged();
    m_merging = false;
}

void Sync::dataReplaced()
{
    m_account.setConflicts({});
    if (enabled() || !m_account.account().folder.isEmpty()) {
        m_account.setReplaced(SyncAccount::Toggle::On);
    }
    m_error.clear();        // a failed run before this says nothing about the new state
    setEnabled(false);
    emit accountChanged();
    emit stateChanged();
}

void Sync::setError(const QString &error)
{
    if (m_error != error) {
        m_error = error;
        emit stateChanged();
    }
}

QVariantList Sync::devices() const
{
    QVariantList list;
    for (const SyncAccount::Device &d : m_account.devices()) {
        list << QVariantMap{{QStringLiteral("id"), d.id}, {QStringLiteral("name"), d.name},
                            {QStringLiteral("synced"), d.synced}};
    }
    return list;
}

QVariantList Sync::conflicts() const
{
    const QList<TableMerge::Conflict> open = m_account.conflicts();
    if (open.isEmpty()) {
        return {};
    }

    // "Mine" as the row is now: changed again after the sync, that is what a choice keeps or drops.
    PracticeLog log;
    CourseBook book;
    const bool readable = log.open(m_practice) && book.open(m_courses);
    QVariantList list;
    int index = 0;
    for (TableMerge::Conflict c : open) {
        if (readable) {
            c.mine = c.table == kNotes ? currentText(log.notes(c.parent), c) : currentText(book.entries(c.parent), c);
        }
        list << QVariantMap{{QStringLiteral("index"), index++},
                            {QStringLiteral("kind"), c.table == kNotes ? QStringLiteral("note") : QStringLiteral("entry")},
                            {QStringLiteral("parent"), c.parent},
                            {QStringLiteral("mine"), c.mine},
                            {QStringLiteral("theirs"), c.theirs},
                            {QStringLiteral("mineChanged"), c.mineChanged},
                            {QStringLiteral("theirsChanged"), c.theirsChanged},
                            {QStringLiteral("device"), c.theirDevice}};
    }
    return list;
}

// The choice as a new change (now): it wins on every device at the next sync. "Both" keeps this
// text and adds the other as a new note of the same technique or notebook.
bool Sync::resolve(int index, Choice choice)
{
    QList<TableMerge::Conflict> list = m_account.conflicts();
    if (m_busy || index < 0 || index >= list.size()) {
        return false;
    }
    const TableMerge::Conflict c = list.at(index);
    const QDateTime now = QDateTime::currentDateTimeUtc();
    bool ok = false;
    if (c.table == kNotes) {
        PracticeLog log;
        ok = log.open(m_practice);
        const QString mine = ok ? currentText(log.notes(c.parent), c) : QString();
        ok = ok && log.changeNote(c.uid, choice == Choice::Theirs ? c.theirs : mine, now)
             && (choice != Choice::Both || !log.addNote(c.parent, c.theirs, now, deviceId()).uid.isEmpty());
        m_error = ok ? QString() : log.errorString();
    } else {
        CourseBook book;
        ok = book.open(m_courses);
        const QString mine = ok ? currentText(book.entries(c.parent), c) : QString();
        ok = ok && book.changeEntry(c.uid, choice == Choice::Theirs ? c.theirs : mine, now)
             && (choice != Choice::Both || !book.addEntry(c.parent, c.theirs, now, deviceId()).uid.isEmpty());
        m_error = ok ? QString() : book.errorString();
    }
    if (ok) {
        list.removeAt(index);
        m_account.setConflicts(list);
        announce();
        if (enabled()) {
            m_soon.start(kAfterChangeMs);
        }
    }
    emit stateChanged();
    return ok;
}
