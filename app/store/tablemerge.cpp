#include "tablemerge.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSet>
#include <QSqlQuery>
#include <QTimeZone>

namespace {

const QString kOther = QStringLiteral("other");

bool failed(const QSqlQuery &q, QString *error)
{
    *error = q.lastError().text();
    return false;
}

// One table: the rows this copy lacks, then (editable) the newer ones.
bool mergeTable(QSqlDatabase &db, const TableMerge::Table &t, const TableMerge::Watch &watch,
                TableMerge::Result *result, QString *error)
{
    QSqlQuery q(db);
    const QString uid = t.columns.first();
    const QString all = t.columns.join(u", ");
    if (!q.exec(QStringLiteral("INSERT INTO main.%1 (%2) SELECT %2 FROM %3.%1 WHERE %4 NOT IN (SELECT %4 FROM main.%1)")
                    .arg(t.name, all, kOther, uid))) {
        return failed(q, error);
    }
    result->added[t.name] = q.numRowsAffected();
    if (t.kind == TableMerge::Kind::Log) {
        return true;
    }

    QString newer = QStringLiteral("o.%1 = main.%2.%1 AND o.changed > main.%2.changed").arg(uid, t.name);

    // Conflicts: both texts changed (since the last sync, or still held), media alike, none deleted.
    if (!t.text.isEmpty() && watch.since.isValid()) {
        const QString clash = QStringLiteral(
            "(main.%1.%2 IN (SELECT uid FROM temp.held) OR (main.%1.changed > %3 AND o.changed > %3))"
            " AND main.%1.deleted = 0 AND o.deleted = 0 AND o.%4 <> main.%1.%4%5")
            .arg(t.name, uid, QString::number(watch.since.toMSecsSinceEpoch()), t.text,
                 t.media.isEmpty() ? QString() : QStringLiteral(" AND o.%1 = main.%2.%1").arg(t.media, t.name));
        if (!q.exec(QStringLiteral("SELECT main.%1.%2, main.%1.%3, main.%1.%4, o.%4, main.%1.changed, o.changed, o.device"
                                   " FROM main.%1 JOIN %5.%1 o ON o.%2 = main.%1.%2 WHERE %6")
                        .arg(t.name, uid, t.parent, t.text, kOther, clash))) {
            return failed(q, error);
        }
        while (q.next()) {
            result->conflicts << TableMerge::Conflict{
                t.name, q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString(),
                QDateTime::fromMSecsSinceEpoch(q.value(4).toLongLong(), QTimeZone::UTC),
                QDateTime::fromMSecsSinceEpoch(q.value(5).toLongLong(), QTimeZone::UTC), q.value(6).toString()};
        }
        newer += QStringLiteral(" AND NOT (%1)").arg(clash);
    }

    const QString rest = t.columns.mid(1).join(u", ");
    if (!q.exec(QStringLiteral("UPDATE main.%1 SET (%2) = (SELECT %2 FROM %3.%1 o WHERE o.%4 = main.%1.%4)"
                               " WHERE EXISTS (SELECT 1 FROM %3.%1 o WHERE %5)")
                    .arg(t.name, rest, kOther, uid, newer))) {
        return failed(q, error);
    }
    result->changed[t.name] = q.numRowsAffected();
    return true;
}

// The uids of open conflicts, in a table of the connection (the merge's statements read it).
bool hold(QSqlDatabase &db, const QStringList &held, QString *error)
{
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("CREATE TEMP TABLE IF NOT EXISTS held (uid TEXT PRIMARY KEY)"))
        || !q.exec(QStringLiteral("DELETE FROM temp.held"))) {
        return failed(q, error);
    }
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO temp.held (uid) VALUES (?)"));
    for (const QString &uid : held) {
        q.addBindValue(uid);
        if (!q.exec()) {
            return failed(q, error);
        }
    }
    return true;
}

// The media files the rows that are not deleted name.
QSet<QString> liveMedia(QSqlDatabase &db, const QList<TableMerge::Table> &tables, bool *ok, QString *error)
{
    QSet<QString> names;
    QSqlQuery q(db);
    for (const TableMerge::Table &t : tables) {
        if (t.media.isEmpty()) {
            continue;
        }
        if (!q.exec(QStringLiteral("SELECT %1 FROM main.%2 WHERE deleted = 0 AND %1 <> ''").arg(t.media, t.name))) {
            *ok = failed(q, error);
            return {};
        }
        while (q.next()) {
            names.insert(q.value(0).toString());
        }
    }
    return names;
}

} // namespace

bool TableMerge::run(QSqlDatabase &db, const QString &otherPath, const QList<Table> &tables, Mode mode,
                     Result *result, QString *error, After after, const Watch &watch)
{
    *result = {};
    QSqlQuery q(db);
    q.prepare(QStringLiteral("ATTACH DATABASE ? AS %1").arg(kOther));
    q.addBindValue(otherPath);
    if (!q.exec()) {
        return failed(q, error);
    }
    q.finish();

    bool ok = db.transaction();
    if (!ok) {
        *error = db.lastError().text();
    }
    const QSet<QString> before = ok ? liveMedia(db, tables, &ok, error) : QSet<QString>();
    ok = ok && hold(db, watch.held, error);
    for (const Table &t : tables) {
        ok = ok && mergeTable(db, t, watch, result, error);
    }
    ok = ok && (!after || after(db, error));

    // The media the rows name after the merge: only those come from the backup (a deleted note's
    // photo not); what they named before and no more goes.
    const QSet<QString> named = ok ? liveMedia(db, tables, &ok, error) : QSet<QString>();
    result->media = named.values();
    result->orphans = (before - named).values();
    if (ok && mode == Mode::Apply) {
        ok = db.commit();
        if (!ok) {
            *error = db.lastError().text();
        }
    }
    if (!ok || mode == Mode::Preview) {
        db.rollback();
    }

    q.exec(QStringLiteral("DETACH DATABASE %1").arg(kOther));
    return ok;
}
