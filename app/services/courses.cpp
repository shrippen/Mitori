#include "courses.h"

#include "device.h"
#include "devicecalendar.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

// The association's public course calendar.
const QUrl kCalendar(QStringLiteral("https://cal.arianw.de/dav/calendars/shinki/Lehrgaenge?export"));
// A load counts as fresh for this long (Reload::Older): opening the page often costs no traffic.
constexpr qint64 kFreshSeconds = 6 * 60 * 60;
constexpr int kTimeoutMs = 20 * 1000;

QString dataDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QVariantMap courseMap(const CourseBook::Course &c, int entries)
{
    return {{QStringLiteral("uid"), c.uid},
            {QStringLiteral("calendarUid"), c.calendarUid},
            {QStringLiteral("title"), c.title},
            {QStringLiteral("first"), c.first.toString(Qt::ISODate)},
            {QStringLiteral("last"), c.last.toString(Qt::ISODate)},
            {QStringLiteral("place"), c.place},
            {QStringLiteral("teacher"), c.teacher},
            {QStringLiteral("info"), c.info},
            {QStringLiteral("announcement"), c.announcement},
            {QStringLiteral("entries"), entries}};
}

} // namespace

// MITORI_COURSES_CALENDAR: another source (tests: a local file).
Courses::Courses(QObject *parent)
    : Courses(defaultPath(),
              qEnvironmentVariableIsSet("MITORI_COURSES_CALENDAR") ? QUrl(qEnvironmentVariable("MITORI_COURSES_CALENDAR")) : kCalendar,
              dataDir() + QStringLiteral("/lehrgaenge.ics"), parent)
{
}

Courses::Courses(const QString &path, const QUrl &calendar, const QString &cache, QObject *parent)
    : QObject(parent)
    , m_book(std::make_unique<CourseBook>())
    , m_source(calendar)
    , m_cache(cache)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    m_open = m_book->open(path);
    if (!m_open) {
        m_error = tr("The course notebooks could not be opened: %1").arg(m_book->errorString());
        qWarning("Courses: %s", qPrintable(m_error));
    }
    loadCache();
}

Courses::~Courses() = default;

QString Courses::defaultPath()
{
    return dataDir() + QStringLiteral("/lehrgaenge.sqlite");
}

QString Courses::teacherOf(const QString &description)
{
    static const QRegularExpression with(QStringLiteral("^\\s*mit\\s+(.+?)\\s*$"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = with.match(description.section(QLatin1Char('\n'), 0, 0));
    return m.hasMatch() ? m.captured(1) : QString();
}

// The calendar as it was last loaded: at once, also offline.
void Courses::loadCache()
{
    QFile file(m_cache);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    m_events = IcsReader::read(file.readAll());
    m_loaded = QFileInfo(file).lastModified(QTimeZone::UTC);
    m_state = CalendarState::Ready;
}

void Courses::reload(Reload when)
{
    if (m_state == CalendarState::Loading) {
        return;
    }
    if (when == Reload::Older && m_loaded.isValid() && m_loaded.secsTo(QDateTime::currentDateTimeUtc()) < kFreshSeconds) {
        return;
    }
    if (!m_network) {
        m_network = new QNetworkAccessManager(this);
        connect(m_network, &QNetworkAccessManager::finished, this, &Courses::received);
    }
    QNetworkRequest request(m_source);
    request.setTransferTimeout(kTimeoutMs);
    m_network->get(request);
    m_state = CalendarState::Loading;
    emit calendarChanged();
}

// A calendar without events (an error page, an empty answer) does not replace the one on the
// device.
void Courses::received(QNetworkReply *reply)
{
    reply->deleteLater();
    const QByteArray data = reply->error() == QNetworkReply::NoError ? reply->readAll() : QByteArray();
    const QList<IcsReader::Event> events = IcsReader::read(data);
    if (events.isEmpty()) {
        qWarning("Courses: the calendar could not be loaded: %s", qPrintable(reply->errorString()));
        m_state = CalendarState::Failed;
        emit calendarChanged();
        return;
    }
    QDir().mkpath(QFileInfo(m_cache).absolutePath());
    QSaveFile file(m_cache);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.commit();
    }
    m_events = events;
    m_loaded = QDateTime::currentDateTimeUtc();
    m_state = CalendarState::Ready;
    emit calendarChanged();
}

QVariantList Courses::upcoming() const
{
    // Notebooks by their event, so a row shows its notes.
    QHash<QString, QString> notebookOf;
    const QHash<QString, int> counts = m_open ? m_book->entryCounts() : QHash<QString, int>();
    QHash<QString, int> entriesOf;
    if (m_open) {
        for (const CourseBook::Course &c : m_book->courses()) {
            if (!c.calendarUid.isEmpty()) {
                notebookOf.insert(c.calendarUid, c.uid);
                entriesOf.insert(c.calendarUid, counts.value(c.uid));
            }
        }
    }

    QList<IcsReader::Event> coming;
    const QDate today = QDate::currentDate();
    for (const IcsReader::Event &e : m_events) {
        if (e.last >= today) {
            coming << e;
        }
    }
    std::stable_sort(coming.begin(), coming.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    QVariantList result;
    for (const IcsReader::Event &e : coming) {
        result << QVariantMap{{QStringLiteral("uid"), e.uid},
                              {QStringLiteral("title"), e.title},
                              {QStringLiteral("first"), e.first.toString(Qt::ISODate)},
                              {QStringLiteral("last"), e.last.toString(Qt::ISODate)},
                              {QStringLiteral("place"), e.place},
                              {QStringLiteral("teacher"), teacherOf(e.description)},
                              {QStringLiteral("info"), e.description},
                              {QStringLiteral("announcement"), e.announcement},
                              {QStringLiteral("notebook"), notebookOf.value(e.uid)},
                              {QStringLiteral("entries"), entriesOf.value(e.uid)}};
    }
    return result;
}

QVariantList Courses::notebooks() const
{
    QVariantList result;
    if (!m_open) {
        return result;
    }
    const QHash<QString, int> counts = m_book->entryCounts();
    for (const CourseBook::Course &c : m_book->courses()) {
        result << courseMap(c, counts.value(c.uid));
    }
    return result;
}

QVariantMap Courses::notebook(const QString &uid) const
{
    if (!m_open) {
        return {};
    }
    for (const CourseBook::Course &c : m_book->courses()) {
        if (c.uid == uid) {
            return courseMap(c, m_book->entryCounts().value(uid));
        }
    }
    return {};
}

QString Courses::notebookFor(const QString &eventUid)
{
    if (!m_open) {
        return {};
    }
    const CourseBook::Course known = m_book->courseForEvent(eventUid);
    if (!known.uid.isEmpty()) {
        return known.uid;
    }
    const auto it = std::find_if(m_events.cbegin(), m_events.cend(), [&](const auto &e) { return e.uid == eventUid; });
    if (it == m_events.cend()) {
        return {};
    }
    // A copy of the event: the notebook keeps it when the calendar changes or drops the event.
    CourseBook::Course c;
    c.calendarUid = it->uid;
    c.title = it->title;
    c.first = it->first;
    c.last = it->last;
    c.place = it->place;
    c.teacher = teacherOf(it->description);
    c.info = it->description;
    c.announcement = it->announcement;
    const CourseBook::Course made = m_book->addCourse(c, QDateTime::currentDateTimeUtc(), deviceId());
    saved(!made.uid.isEmpty(), tr("The notebook could not be made: %1"));
    return made.uid;
}

QString Courses::create(const QString &title, const QString &first, const QString &last, const QString &place,
                        const QString &teacher)
{
    if (!m_open) {
        return {};
    }
    CourseBook::Course c;
    c.title = title;
    c.first = QDate::fromString(first, Qt::ISODate);
    c.last = QDate::fromString(last, Qt::ISODate);
    c.place = place.trimmed();
    c.teacher = teacher.trimmed();
    const CourseBook::Course made = m_book->addCourse(c, QDateTime::currentDateTimeUtc(), deviceId());
    saved(!made.uid.isEmpty(), tr("The notebook could not be made: %1"));
    return made.uid;
}

bool Courses::remove(const QString &uid)
{
    return m_open && saved(m_book->deleteCourse(uid, QDateTime::currentDateTimeUtc()), tr("The notebook could not be deleted: %1"));
}

QVariantList Courses::entries(const QString &course) const
{
    QVariantList result;
    if (!m_open) {
        return result;
    }
    for (const CourseBook::Entry &e : m_book->entries(course)) {
        result << QVariantMap{{QStringLiteral("uid"), e.uid},
                              {QStringLiteral("text"), e.text},
                              {QStringLiteral("created"), e.created.toLocalTime()},
                              {QStringLiteral("changed"), e.changed.toLocalTime()}};
    }
    return result;
}

bool Courses::addEntry(const QString &course, const QString &text)
{
    if (!m_open || text.trimmed().isEmpty()) {
        return false;
    }
    const CourseBook::Entry e = m_book->addEntry(course, text, QDateTime::currentDateTimeUtc(), deviceId());
    return saved(!e.uid.isEmpty(), tr("The entry could not be saved: %1"));
}

bool Courses::changeEntry(const QString &uid, const QString &text)
{
    if (!m_open || text.trimmed().isEmpty()) {
        return false;
    }
    return saved(m_book->changeEntry(uid, text, QDateTime::currentDateTimeUtc()), tr("The entry could not be saved: %1"));
}

bool Courses::deleteEntry(const QString &uid)
{
    return m_open && saved(m_book->deleteEntry(uid, QDateTime::currentDateTimeUtc()), tr("The entry could not be deleted: %1"));
}

bool Courses::addToDeviceCalendar(const QString &uid)
{
    const QVariantMap c = notebook(uid);
    if (c.isEmpty()) {
        return false;
    }
    return DeviceCalendar::add({c.value(QStringLiteral("calendarUid")).toString(), c.value(QStringLiteral("title")).toString(),
                                QDate::fromString(c.value(QStringLiteral("first")).toString(), Qt::ISODate),
                                QDate::fromString(c.value(QStringLiteral("last")).toString(), Qt::ISODate),
                                c.value(QStringLiteral("place")).toString(), c.value(QStringLiteral("info")).toString()});
}

// After a change: the new revision, or the error with what went wrong ("%1" the store's text).
bool Courses::saved(bool ok, const QString &what)
{
    if (!ok) {
        m_error = what.arg(m_book->errorString());
        qWarning("Courses: %s", qPrintable(m_error));
        emit errorChanged();
        return false;
    }
    ++m_revision;
    emit changed();
    emit calendarChanged();      // the rows of the calendar show their notebook
    if (!m_error.isEmpty()) {
        m_error.clear();
        emit errorChanged();
    }
    return true;
}
