#include "practicelog.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include <QUuid>
#include <QVariant>

namespace {

PracticeLog::Rating rating(const QSqlQuery &q)
{
    PracticeLog::Rating r;
    r.uid = q.value(0).toString();
    r.entry = q.value(1).toString();
    r.level = q.value(2).toInt();
    r.at = QDateTime::fromMSecsSinceEpoch(q.value(3).toLongLong(), QTimeZone::UTC);
    r.device = q.value(4).toString();
    return r;
}

} // namespace

PracticeLog::PracticeLog()
    : m_connection(QStringLiteral("practicelog-") + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

PracticeLog::~PracticeLog()
{
    if (QSqlDatabase::contains(m_connection)) {
        QSqlDatabase::database(m_connection).close();
        QSqlDatabase::removeDatabase(m_connection);
    }
}

bool PracticeLog::open(const QString &path)
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
bool PracticeLog::migrate()
{
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA user_version"));
    const int version = q.next() ? q.value(0).toInt() : 0;
    if (version > schemaVersion) {
        m_error = QCoreApplication::translate("PracticeLog", "it comes from a newer Mitori (schema %1); update Mitori").arg(version);
        return false;
    }
    if (version < 1) {
        const bool ok = q.exec(QStringLiteral(
                            "CREATE TABLE IF NOT EXISTS ratings ("
                            " uid TEXT PRIMARY KEY, entry TEXT NOT NULL, level INTEGER NOT NULL,"
                            " at INTEGER NOT NULL, device TEXT NOT NULL)"))
            && q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS ratings_entry ON ratings (entry, at)"))
            && q.exec(QStringLiteral("PRAGMA user_version = 1"));
        if (!ok) {
            m_error = q.lastError().text();
            return false;
        }
    }
    return true;
}

PracticeLog::Rating PracticeLog::add(const QString &entry, int level, const QDateTime &at, const QString &device)
{
    Rating r;
    if (entry.isEmpty() || level < 1 || level > 4) {
        return r;
    }
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("INSERT INTO ratings (uid, entry, level, at, device) VALUES (?, ?, ?, ?, ?)"));
    const QString uid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    q.addBindValue(uid);
    q.addBindValue(entry);
    q.addBindValue(level);
    q.addBindValue(at.toMSecsSinceEpoch());
    q.addBindValue(device);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return r;
    }
    r = {uid, entry, level, at.toUTC(), device};
    return r;
}

QHash<QString, PracticeLog::Rating> PracticeLog::latest() const
{
    QHash<QString, Rating> result;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    // Oldest first: a later rating of the same entry replaces the earlier one.
    q.exec(QStringLiteral("SELECT uid, entry, level, at, device FROM ratings ORDER BY at, rowid"));
    while (q.next()) {
        const Rating r = rating(q);
        // Only levels 1..4 count (a merged or edited log may carry others).
        if (r.level >= 1 && r.level <= 4) {
            result.insert(r.entry, r);
        }
    }
    return result;
}

QHash<QString, QDate> PracticeLog::firstRated() const
{
    QHash<QString, QDate> result;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.exec(QStringLiteral("SELECT entry, MIN(at) FROM ratings WHERE level BETWEEN 1 AND 4 GROUP BY entry"));
    while (q.next()) {
        result.insert(q.value(0).toString(),
                      QDateTime::fromMSecsSinceEpoch(q.value(1).toLongLong(), QTimeZone::UTC).toLocalTime().date());
    }
    return result;
}

QList<PracticeLog::Rating> PracticeLog::history(const QString &entry) const
{
    QList<Rating> result;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("SELECT uid, entry, level, at, device FROM ratings WHERE entry = ? ORDER BY at, rowid"));
    q.addBindValue(entry);
    q.exec();
    while (q.next()) {
        const Rating r = rating(q);
        if (r.level >= 1 && r.level <= 4) {
            result << r;
        }
    }
    return result;
}
