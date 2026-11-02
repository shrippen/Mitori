#include "courses.h"

#include "device.h"
#include "mediastore.h"
#include "devicecalendar.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
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

} // namespace

QVariantMap Courses::courseMap(const CourseBook::Course &c, int entries) const
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
            {QStringLiteral("entries"), entries},
            {QStringLiteral("examDiscipline"), c.examDiscipline},
            {QStringLiteral("examGrade"), c.examGrade},
            {QStringLiteral("examName"), examName(c.examDiscipline, c.examGrade)}};
}

// MITORI_COURSES_CALENDAR: another source (tests: a local file).
Courses::Courses(QObject *parent)
    : Courses(defaultPath(),
              qEnvironmentVariableIsSet("MITORI_COURSES_CALENDAR") ? QUrl(qEnvironmentVariable("MITORI_COURSES_CALENDAR")) : kCalendar,
              dataDir() + QStringLiteral("/lehrgaenge.ics"), Catalog::bundled(), parent)
{
}

Courses::Courses(const QString &path, const QUrl &calendar, const QString &cache, const Catalog &catalog, QObject *parent)
    : QObject(parent)
    , m_book(std::make_unique<CourseBook>())
    , m_source(calendar)
    , m_cache(cache)
{
    // The grades one can take, with their waiting times (`wartezeit`, tools/daten.py).
    for (const QVariant &dv : catalog.disciplines()) {
        const QVariantMap d = dv.toMap();
        const QString id = d.value(QStringLiteral("id")).toString();
        m_disciplineNames.insert(id, d.value(QStringLiteral("kurzname")).toString());
        QList<Step> &list = m_steps[id];
        for (const QVariant &gv : d.value(QStringLiteral("grade")).toList()) {
            const QVariantMap g = gv.toMap();
            if (!g.value(QStringLiteral("pruefbar"), true).toBool()) {
                continue;
            }
            const QVariantMap wait = g.value(QStringLiteral("wartezeit")).toMap();
            list << Step{g.value(QStringLiteral("id")).toString(), g.value(QStringLiteral("name")).toString(),
                         wait.value(QStringLiteral("jahre")).toInt(),
                         wait.value(QStringLiteral("nach")).toString() == QLatin1String("beginn")};
        }
    }

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
    if (!m_open) {
        return false;
    }
    const QStringList media = m_book->courseMedia(uid);
    if (!saved(m_book->deleteCourse(uid, QDateTime::currentDateTimeUtc()), tr("The notebook could not be deleted: %1"))) {
        return false;
    }
    for (const QString &name : media) {
        MediaStore::shared().remove(name);
    }
    return true;
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
                              {QStringLiteral("changed"), e.changed.toLocalTime()},
                              {QStringLiteral("media"), e.media}};
    }
    return result;
}

QList<CourseBook::Entry> Courses::allEntries() const
{
    return m_open ? m_book->allEntries() : QList<CourseBook::Entry>();
}

QHash<QString, QString> Courses::courseTitles() const
{
    QHash<QString, QString> titles;
    if (m_open) {
        for (const CourseBook::Course &c : m_book->courses()) {
            titles.insert(c.uid, c.title);
        }
    }
    return titles;
}

bool Courses::addEntry(const QString &course, const QString &text, const QString &media)
{
    if (!m_open || (text.trimmed().isEmpty() && media.isEmpty())) {
        return false;
    }
    const CourseBook::Entry e = m_book->addEntry(course, text, QDateTime::currentDateTimeUtc(), deviceId(), media);
    return saved(!e.uid.isEmpty(), tr("The entry could not be saved: %1"));
}

bool Courses::changeEntry(const QString &uid, const QString &text)
{
    if (!m_open || (text.trimmed().isEmpty() && m_book->entryMedia(uid).isEmpty())) {
        return false;
    }
    return saved(m_book->changeEntry(uid, text, QDateTime::currentDateTimeUtc()), tr("The entry could not be saved: %1"));
}

// The entry stays as a mark (sync), its photo or voice note goes.
bool Courses::deleteEntry(const QString &uid)
{
    if (!m_open || !saved(m_book->deleteEntry(uid, QDateTime::currentDateTimeUtc()), tr("The entry could not be deleted: %1"))) {
        return false;
    }
    MediaStore::shared().remove(m_book->entryMedia(uid));
    return true;
}

QList<Courses::Step> Courses::steps(const QString &discipline) const
{
    return m_steps.value(discipline);
}

QString Courses::gradeName(const QString &discipline, const QString &grade) const
{
    for (const Step &s : steps(discipline)) {
        if (s.id == grade) {
            return s.name;
        }
    }
    return {};
}

// "Aikido 4. Kyu", or "" without an exam.
QString Courses::examName(const QString &discipline, const QString &grade) const
{
    const QString name = gradeName(discipline, grade);
    return name.isEmpty() ? QString() : m_disciplineNames.value(discipline) + QLatin1Char(' ') + name;
}

QVariantList Courses::grades() const
{
    QVariantList result;
    if (!m_open) {
        return result;
    }
    for (const CourseBook::Grade &g : m_book->grades()) {
        result << QVariantMap{{QStringLiteral("uid"), g.uid},
                              {QStringLiteral("discipline"), g.discipline},
                              {QStringLiteral("grade"), g.grade},
                              {QStringLiteral("name"), gradeName(g.discipline, g.grade)},
                              {QStringLiteral("day"), g.day.toString(Qt::ISODate)},
                              {QStringLiteral("place"), g.place}};
    }
    return result;
}

bool Courses::addGrade(const QString &discipline, const QString &grade, const QString &day, const QString &place)
{
    const QDate date = QDate::fromString(day, Qt::ISODate);
    if (!m_open || gradeName(discipline, grade).isEmpty() || !date.isValid()) {
        return false;
    }
    const CourseBook::Grade g = m_book->addGrade({QString(), discipline, grade, date, place}, QDateTime::currentDateTimeUtc(), deviceId());
    return saved(!g.uid.isEmpty(), tr("The grade could not be saved: %1"));
}

bool Courses::removeGrade(const QString &uid)
{
    return m_open && saved(m_book->deleteGrade(uid, QDateTime::currentDateTimeUtc()), tr("The grade could not be deleted: %1"));
}

QVariantList Courses::ladder(const QString &discipline) const
{
    QSet<QString> passed;
    if (m_open) {
        for (const CourseBook::Grade &g : m_book->grades()) {
            if (g.discipline == discipline) {
                passed.insert(g.grade);
            }
        }
    }
    QVariantList result;
    for (const Step &s : steps(discipline)) {
        result << QVariantMap{{QStringLiteral("id"), s.id}, {QStringLiteral("name"), s.name},
                              {QStringLiteral("passed"), passed.contains(s.id)}};
    }
    return result;
}

// The one after the highest passed: 5. Kyu, 3. Kyu entered → 2. Kyu.
QString Courses::nextGrade(const QString &discipline) const
{
    const QVariantList list = ladder(discipline);
    int highest = -1;
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).toMap().value(QStringLiteral("passed")).toBool()) {
            highest = i;
        }
    }
    return highest + 1 < list.size() ? list.at(highest + 1).toMap().value(QStringLiteral("id")).toString() : QString();
}

// 4. Kyu, 1 year after 5. Kyu (passed 2025-03-08): earliest 2026-03-08.
QVariantMap Courses::waiting(const QString &discipline, const QString &grade, const QString &day) const
{
    const QList<Step> list = steps(discipline);
    const auto it = std::find_if(list.cbegin(), list.cend(), [&](const Step &s) { return s.id == grade; });
    if (it == list.cend()) {
        return {};
    }
    QVariantMap result{{QStringLiteral("years"), it->years}};
    if (it->fromStart) {
        result.insert(QStringLiteral("state"), QVariant::fromValue(Wait::FromStart));
        return result;
    }
    if (it == list.cbegin() || it->years == 0) {
        result.insert(QStringLiteral("state"), QVariant::fromValue(Wait::First));
        return result;
    }

    const Step &previous = *(it - 1);
    result.insert(QStringLiteral("previous"), previous.name);
    QDate passed;
    if (m_open) {
        for (const CourseBook::Grade &g : m_book->grades()) {
            if (g.discipline == discipline && g.grade == previous.id) {
                passed = g.day;
            }
        }
    }
    if (!passed.isValid()) {
        result.insert(QStringLiteral("state"), QVariant::fromValue(Wait::Unknown));
        return result;
    }

    const QDate earliest = passed.addYears(it->years);
    result.insert(QStringLiteral("earliest"), earliest.toString(Qt::ISODate));
    const QDate exam = QDate::fromString(day, Qt::ISODate);
    const Wait state = !exam.isValid() ? Wait::Earliest : exam >= earliest ? Wait::Fits : Wait::TooEarly;
    result.insert(QStringLiteral("state"), QVariant::fromValue(state));
    return result;
}

bool Courses::planExam(const QString &uid, const QString &discipline, const QString &grade)
{
    const bool none = grade.isEmpty();
    if (!m_open || (!none && gradeName(discipline, grade).isEmpty())) {
        return false;
    }
    return saved(m_book->planExam(uid, none ? QString() : discipline, grade, QDateTime::currentDateTimeUtc()),
                 tr("The exam could not be planned: %1"));
}

QVariantMap Courses::nextExam(const QString &discipline) const
{
    if (!m_open) {
        return {};
    }
    const QDate today = QDate::currentDate();
    const CourseBook::Course *best = nullptr;
    const QList<CourseBook::Course> list = m_book->courses();
    for (const CourseBook::Course &c : list) {
        if (c.examGrade.isEmpty() || c.last < today || (!discipline.isEmpty() && c.examDiscipline != discipline)) {
            continue;
        }
        if (!best || c.first < best->first) {
            best = &c;
        }
    }
    if (!best) {
        return {};
    }
    return {{QStringLiteral("course"), best->uid},
            {QStringLiteral("title"), best->title},
            {QStringLiteral("place"), best->place},
            {QStringLiteral("first"), best->first.toString(Qt::ISODate)},
            {QStringLiteral("discipline"), best->examDiscipline},
            {QStringLiteral("grade"), best->examGrade},
            {QStringLiteral("name"), examName(best->examDiscipline, best->examGrade)},
            {QStringLiteral("days"), std::max<qint64>(0, today.daysTo(best->first))}};
}

bool Courses::addToDeviceCalendar(const QString &uid)
{
    const DeviceCalendar::Event e = deviceEvent(uid);
    return !e.title.isEmpty() && DeviceCalendar::add(e);
}

DeviceCalendar::Event Courses::deviceEvent(const QString &uid) const
{
    const QVariantMap c = notebook(uid);
    if (c.isEmpty()) {
        return {};
    }
    // The calendar event's uid, else the notebook's (a course of one's own): RFC 5545 needs one.
    const QString eventUid = c.value(QStringLiteral("calendarUid")).toString();
    return DeviceCalendar::Event{eventUid.isEmpty() ? uid : eventUid, c.value(QStringLiteral("title")).toString(),
                                QDate::fromString(c.value(QStringLiteral("first")).toString(), Qt::ISODate),
                                QDate::fromString(c.value(QStringLiteral("last")).toString(), Qt::ISODate),
                                c.value(QStringLiteral("place")).toString(), c.value(QStringLiteral("info")).toString()};
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
