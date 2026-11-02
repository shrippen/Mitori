#include "coursebook.h"

#include <QCoreApplication>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include <QUuid>
#include <QVariant>

namespace {

const QString kDateFormat = QStringLiteral("yyyy-MM-dd");
const QString kCourseColumns = QStringLiteral("uid, calendar_uid, title, first, last, place, teacher, info, announcement,"
                                              " created, changed, device, deleted, exam_discipline, exam_grade");
const QString kGradeColumns = QStringLiteral("uid, discipline, grade, day, place, created, changed, device, deleted");

QDateTime utc(const QVariant &ms)
{
    return QDateTime::fromMSecsSinceEpoch(ms.toLongLong(), QTimeZone::UTC);
}

CourseBook::Course course(const QSqlQuery &q)
{
    CourseBook::Course c;
    c.uid = q.value(0).toString();
    c.calendarUid = q.value(1).toString();
    c.title = q.value(2).toString();
    c.first = QDate::fromString(q.value(3).toString(), kDateFormat);
    c.last = QDate::fromString(q.value(4).toString(), kDateFormat);
    c.place = q.value(5).toString();
    c.teacher = q.value(6).toString();
    c.info = q.value(7).toString();
    c.announcement = q.value(8).toString();
    c.created = utc(q.value(9));
    c.changed = utc(q.value(10));
    c.device = q.value(11).toString();
    c.deleted = q.value(12).toBool();
    c.examDiscipline = q.value(13).toString();
    c.examGrade = q.value(14).toString();
    return c;
}

CourseBook::Grade grade(const QSqlQuery &q)
{
    CourseBook::Grade g;
    g.uid = q.value(0).toString();
    g.discipline = q.value(1).toString();
    g.grade = q.value(2).toString();
    g.day = QDate::fromString(q.value(3).toString(), kDateFormat);
    g.place = q.value(4).toString();
    g.created = utc(q.value(5));
    g.changed = utc(q.value(6));
    g.device = q.value(7).toString();
    g.deleted = q.value(8).toBool();
    return g;
}

CourseBook::Entry entry(const QSqlQuery &q)
{
    CourseBook::Entry e;
    e.uid = q.value(0).toString();
    e.course = q.value(1).toString();
    e.text = q.value(2).toString();
    e.created = utc(q.value(3));
    e.changed = utc(q.value(4));
    e.device = q.value(5).toString();
    e.deleted = q.value(6).toBool();
    return e;
}

// A null QString binds as NULL; the columns hold "" for nothing.
QVariant bound(const QString &s)
{
    return s.isNull() ? QVariant(QString(QLatin1String(""))) : QVariant(s);
}

QString newUid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace

CourseBook::CourseBook()
    : m_connection(QStringLiteral("coursebook-") + newUid())
{
}

CourseBook::~CourseBook()
{
    if (QSqlDatabase::contains(m_connection)) {
        QSqlDatabase::database(m_connection).close();
        QSqlDatabase::removeDatabase(m_connection);
    }
}

bool CourseBook::open(const QString &path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    db.setDatabaseName(path);
    if (!db.open()) {
        m_error = db.lastError().text();
        return false;
    }
    return migrate();
}

// user_version holds the schema; each step brings it one version further.
bool CourseBook::migrate()
{
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA user_version"));
    const int version = q.next() ? q.value(0).toInt() : 0;
    if (version > schemaVersion) {
        m_error = QCoreApplication::translate("CourseBook", "it comes from a newer Mitori (schema %1); update Mitori").arg(version);
        return false;
    }
    if (version < 1) {
        const bool ok = db.transaction()
            && q.exec(QStringLiteral("CREATE TABLE courses (uid TEXT PRIMARY KEY, calendar_uid TEXT NOT NULL DEFAULT '',"
                                     " title TEXT NOT NULL, first TEXT NOT NULL, last TEXT NOT NULL, place TEXT NOT NULL DEFAULT '',"
                                     " teacher TEXT NOT NULL DEFAULT '', info TEXT NOT NULL DEFAULT '',"
                                     " announcement TEXT NOT NULL DEFAULT '', created INTEGER NOT NULL, changed INTEGER NOT NULL,"
                                     " device TEXT NOT NULL, deleted INTEGER NOT NULL DEFAULT 0)"))
            && q.exec(QStringLiteral("CREATE TABLE entries (uid TEXT PRIMARY KEY, course TEXT NOT NULL, text TEXT NOT NULL,"
                                     " created INTEGER NOT NULL, changed INTEGER NOT NULL, device TEXT NOT NULL,"
                                     " deleted INTEGER NOT NULL DEFAULT 0)"))
            && q.exec(QStringLiteral("CREATE INDEX entries_course ON entries (course)"))
            && q.exec(QStringLiteral("PRAGMA user_version = 1"))
            && db.commit();
        if (!ok) {
            m_error = q.lastError().text();
            db.rollback();
            return false;
        }
    }

    // 2: the user's grades and an exam planned on a course.
    if (version < 2) {
        const bool ok = db.transaction()
            && q.exec(QStringLiteral("ALTER TABLE courses ADD COLUMN exam_discipline TEXT NOT NULL DEFAULT ''"))
            && q.exec(QStringLiteral("ALTER TABLE courses ADD COLUMN exam_grade TEXT NOT NULL DEFAULT ''"))
            && q.exec(QStringLiteral("CREATE TABLE grades (uid TEXT PRIMARY KEY, discipline TEXT NOT NULL, grade TEXT NOT NULL,"
                                     " day TEXT NOT NULL, place TEXT NOT NULL DEFAULT '', created INTEGER NOT NULL,"
                                     " changed INTEGER NOT NULL, device TEXT NOT NULL, deleted INTEGER NOT NULL DEFAULT 0)"))
            && q.exec(QStringLiteral("PRAGMA user_version = 2"))
            && db.commit();
        if (!ok) {
            m_error = q.lastError().text();
            db.rollback();
            return false;
        }
    }
    return true;
}

CourseBook::Course CourseBook::addCourse(Course c, const QDateTime &at, const QString &device)
{
    m_error.clear();
    if (c.title.trimmed().isEmpty() || !c.first.isValid()) {
        m_error = QCoreApplication::translate("CourseBook", "a course needs a title and a first day");
        return {};
    }
    c.uid = newUid();
    c.created = at.toUTC();
    c.changed = c.created;
    c.device = device;
    c.deleted = false;
    if (!c.last.isValid() || c.last < c.first) {
        c.last = c.first;
    }

    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("INSERT INTO courses (%1) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, ?, ?)").arg(kCourseColumns));
    for (const QVariant &v : {QVariant(c.uid), bound(c.calendarUid), QVariant(c.title.trimmed()),
                              QVariant(c.first.toString(kDateFormat)), QVariant(c.last.toString(kDateFormat)),
                              bound(c.place), bound(c.teacher), bound(c.info), bound(c.announcement),
                              QVariant(c.created.toMSecsSinceEpoch()), QVariant(c.changed.toMSecsSinceEpoch()),
                              bound(device), bound(c.examDiscipline), bound(c.examGrade)}) {
        q.addBindValue(v);
    }
    if (!q.exec()) {
        m_error = q.lastError().text();
        return {};
    }
    c.title = c.title.trimmed();
    return c;
}

QList<CourseBook::Course> CourseBook::courses() const
{
    QList<Course> result;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.exec(QStringLiteral("SELECT %1 FROM courses WHERE deleted = 0 ORDER BY first DESC, created DESC").arg(kCourseColumns));
    while (q.next()) {
        result << course(q);
    }
    return result;
}

CourseBook::Course CourseBook::courseForEvent(const QString &calendarUid) const
{
    if (calendarUid.isEmpty()) {
        return {};
    }
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("SELECT %1 FROM courses WHERE calendar_uid = ? AND deleted = 0 ORDER BY created LIMIT 1").arg(kCourseColumns));
    q.addBindValue(calendarUid);
    return q.exec() && q.next() ? course(q) : Course();
}

// The notebook and its entries, in one step: the entries go as marks too (sync).
bool CourseBook::deleteCourse(const QString &uid, const QDateTime &at)
{
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    if (!db.transaction()) {
        m_error = db.lastError().text();
        return false;
    }
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE entries SET deleted = 1, changed = ? WHERE course = ? AND deleted = 0"));
    q.addBindValue(at.toUTC().toMSecsSinceEpoch());
    q.addBindValue(uid);
    if (!q.exec() || !mark(QStringLiteral("courses"), uid, at)) {
        if (m_error.isEmpty()) {
            m_error = q.lastError().text();
        }
        db.rollback();
        return false;
    }
    return db.commit();
}

bool CourseBook::planExam(const QString &uid, const QString &discipline, const QString &grade, const QDateTime &at)
{
    m_error.clear();
    const bool none = discipline.isEmpty() || grade.isEmpty();
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("UPDATE courses SET exam_discipline = ?, exam_grade = ?, changed = ? WHERE uid = ? AND deleted = 0"));
    q.addBindValue(none ? QString(QLatin1String("")) : discipline);
    q.addBindValue(none ? QString(QLatin1String("")) : grade);
    q.addBindValue(at.toUTC().toMSecsSinceEpoch());
    q.addBindValue(uid);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

// A grade passed again (a corrected day or place) keeps its row: one per discipline and grade.
CourseBook::Grade CourseBook::addGrade(Grade g, const QDateTime &at, const QString &device)
{
    m_error.clear();
    if (g.discipline.isEmpty() || g.grade.isEmpty() || !g.day.isValid()) {
        m_error = QCoreApplication::translate("CourseBook", "a grade needs a discipline, a grade and a day");
        return {};
    }
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT %1 FROM grades WHERE discipline = ? AND grade = ? AND deleted = 0").arg(kGradeColumns));
    q.addBindValue(g.discipline);
    q.addBindValue(g.grade);
    const bool known = q.exec() && q.next();
    const Grade old = known ? grade(q) : Grade();
    q.finish();

    g.uid = known ? old.uid : newUid();
    g.created = known ? old.created : at.toUTC();
    g.changed = at.toUTC();
    g.device = device;
    g.deleted = false;
    g.place = g.place.trimmed();
    if (known) {
        q.prepare(QStringLiteral("UPDATE grades SET day = ?, place = ?, changed = ?, device = ? WHERE uid = ?"));
        for (const QVariant &v : {QVariant(g.day.toString(kDateFormat)), bound(g.place), QVariant(g.changed.toMSecsSinceEpoch()),
                                  bound(device), QVariant(g.uid)}) {
            q.addBindValue(v);
        }
    } else {
        q.prepare(QStringLiteral("INSERT INTO grades (%1) VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0)").arg(kGradeColumns));
        for (const QVariant &v : {QVariant(g.uid), QVariant(g.discipline), QVariant(g.grade), QVariant(g.day.toString(kDateFormat)),
                                  bound(g.place), QVariant(g.created.toMSecsSinceEpoch()), QVariant(g.changed.toMSecsSinceEpoch()),
                                  bound(device)}) {
            q.addBindValue(v);
        }
    }
    if (!q.exec()) {
        m_error = q.lastError().text();
        return {};
    }
    return g;
}

bool CourseBook::deleteGrade(const QString &uid, const QDateTime &at)
{
    return mark(QStringLiteral("grades"), uid, at);
}

QList<CourseBook::Grade> CourseBook::grades() const
{
    QList<Grade> result;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.exec(QStringLiteral("SELECT %1 FROM grades WHERE deleted = 0 ORDER BY day DESC, created DESC").arg(kGradeColumns));
    while (q.next()) {
        result << grade(q);
    }
    return result;
}

CourseBook::Entry CourseBook::addEntry(const QString &courseUid, const QString &text, const QDateTime &at, const QString &device)
{
    m_error.clear();
    Entry e{newUid(), courseUid, text.trimmed(), at.toUTC(), at.toUTC(), device, false};
    if (e.text.isEmpty() || courseUid.isEmpty()) {
        m_error = QCoreApplication::translate("CourseBook", "an entry needs a text and a course");
        return {};
    }
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("INSERT INTO entries (uid, course, text, created, changed, device, deleted)"
                             " VALUES (?, ?, ?, ?, ?, ?, 0)"));
    for (const QVariant &v : {QVariant(e.uid), QVariant(e.course), QVariant(e.text), QVariant(e.created.toMSecsSinceEpoch()),
                              QVariant(e.changed.toMSecsSinceEpoch()), bound(device)}) {
        q.addBindValue(v);
    }
    if (!q.exec()) {
        m_error = q.lastError().text();
        return {};
    }
    return e;
}

bool CourseBook::changeEntry(const QString &uid, const QString &text, const QDateTime &at)
{
    m_error.clear();
    if (text.trimmed().isEmpty()) {
        m_error = QCoreApplication::translate("CourseBook", "an entry needs a text and a course");
        return false;
    }
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("UPDATE entries SET text = ?, changed = ? WHERE uid = ? AND deleted = 0"));
    q.addBindValue(text.trimmed());
    q.addBindValue(at.toUTC().toMSecsSinceEpoch());
    q.addBindValue(uid);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool CourseBook::deleteEntry(const QString &uid, const QDateTime &at)
{
    return mark(QStringLiteral("entries"), uid, at);
}

// A deletion stays as a mark with its time (sync).
bool CourseBook::mark(const QString &table, const QString &uid, const QDateTime &at)
{
    m_error.clear();
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("UPDATE %1 SET deleted = 1, changed = ? WHERE uid = ? AND deleted = 0").arg(table));
    q.addBindValue(at.toUTC().toMSecsSinceEpoch());
    q.addBindValue(uid);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

QList<CourseBook::Entry> CourseBook::entries(const QString &courseUid) const
{
    QList<Entry> result;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("SELECT uid, course, text, created, changed, device, deleted FROM entries"
                             " WHERE course = ? AND deleted = 0 ORDER BY created, rowid"));
    q.addBindValue(courseUid);
    q.exec();
    while (q.next()) {
        result << entry(q);
    }
    return result;
}

QHash<QString, int> CourseBook::entryCounts() const
{
    QHash<QString, int> result;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.exec(QStringLiteral("SELECT course, COUNT(*) FROM entries WHERE deleted = 0 GROUP BY course"));
    while (q.next()) {
        result.insert(q.value(0).toString(), q.value(1).toInt());
    }
    return result;
}
