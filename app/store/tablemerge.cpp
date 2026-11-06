#include "tablemerge.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSet>
#include <QSqlQuery>
#include <QTimeZone>
#include <QUuid>

namespace {

const QString kOther = QStringLiteral("other");
const QString kMain = QStringLiteral("main");

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

    // Conflicts: both changed since the base (a row without one never), or still held; texts
    // differ, media alike, none deleted. Not watched (a backup), none.
    if (!t.text.isEmpty() && (!watch.base.isEmpty() || !watch.held.isEmpty())) {
        const QString base = QStringLiteral("(SELECT changed FROM temp.base WHERE uid = main.%1.%2)").arg(t.name, uid);
        const QString clash = QStringLiteral(
            "(main.%1.%2 IN (SELECT uid FROM temp.held) OR IFNULL(main.%1.changed > %3 AND o.changed > %3, 0))"
            " AND main.%1.deleted = 0 AND o.deleted = 0 AND o.%4 <> main.%1.%4%5")
            .arg(t.name, uid, base, t.text,
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

// The open conflicts and the base versions, in tables of the connection (the merge reads them).
bool hold(QSqlDatabase &db, const TableMerge::Watch &watch, QString *error)
{
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("CREATE TEMP TABLE IF NOT EXISTS held (uid TEXT PRIMARY KEY)"))
        || !q.exec(QStringLiteral("CREATE TEMP TABLE IF NOT EXISTS base (uid TEXT PRIMARY KEY, changed INTEGER NOT NULL)"))
        || !q.exec(QStringLiteral("DELETE FROM temp.held")) || !q.exec(QStringLiteral("DELETE FROM temp.base"))) {
        return failed(q, error);
    }
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO temp.held (uid) VALUES (?)"));
    for (const QString &uid : watch.held) {
        q.addBindValue(uid);
        if (!q.exec()) {
            return failed(q, error);
        }
    }
    q.prepare(QStringLiteral("INSERT OR REPLACE INTO temp.base (uid, changed) VALUES (?, ?)"));
    for (auto it = watch.base.cbegin(); it != watch.base.cend(); ++it) {
        q.addBindValue(it.key());
        q.addBindValue(it.value().toMSecsSinceEpoch());
        if (!q.exec()) {
            return failed(q, error);
        }
    }
    return true;
}

// The media files the rows of one side that are not deleted name.
QSet<QString> liveMedia(QSqlDatabase &db, const QString &side, const QList<TableMerge::Table> &tables, bool *ok,
                        QString *error)
{
    QSet<QString> names;
    QSqlQuery q(db);
    for (const TableMerge::Table &t : tables) {
        if (t.media.isEmpty()) {
            continue;
        }
        if (!q.exec(QStringLiteral("SELECT %1 FROM %2.%3 WHERE deleted = 0 AND %1 <> ''").arg(t.media, side, t.name))) {
            *ok = failed(q, error);
            return {};
        }
        while (q.next()) {
            names.insert(q.value(0).toString());
        }
    }
    return names;
}

using RowSets = QHash<QString, QSet<QString>>;

// The rows of the editable tables that are not deleted, by table.
RowSets liveRows(QSqlDatabase &db, const QList<TableMerge::Table> &tables, bool *ok, QString *error)
{
    RowSets rows;
    QSqlQuery q(db);
    for (const TableMerge::Table &t : tables) {
        if (t.kind == TableMerge::Kind::Log) {
            continue;
        }
        if (!q.exec(QStringLiteral("SELECT %1 FROM main.%2 WHERE deleted = 0").arg(t.columns.first(), t.name))) {
            *ok = failed(q, error);
            return {};
        }
        while (q.next()) {
            rows[t.name].insert(q.value(0).toString());
        }
    }
    return rows;
}

// The rows of the editable tables the other has a newer state of, not a deletion mark.
RowSets newerRows(QSqlDatabase &db, const QList<TableMerge::Table> &tables, bool *ok, QString *error)
{
    RowSets rows;
    QSqlQuery q(db);
    for (const TableMerge::Table &t : tables) {
        if (t.kind == TableMerge::Kind::Log) {
            continue;
        }
        if (!q.exec(QStringLiteral("SELECT o.%1 FROM %2.%3 o JOIN main.%3 m ON m.%1 = o.%1"
                                   " WHERE o.changed > m.changed AND o.deleted = 0")
                        .arg(t.columns.first(), kOther, t.name))) {
            *ok = failed(q, error);
            return {};
        }
        while (q.next()) {
            rows[t.name].insert(q.value(0).toString());
        }
    }
    return rows;
}

// What the merge did to the rows here, by comparing before and after, so that also the clean-up
// counts (an entry going with its notebook), which no comparison of the two copies shows:
// removed  live before, not after
// edited   a newer state taken, live after (a row the clean-up then deleted is only removed)
void countOutcome(const RowSets &before, const RowSets &newer, const RowSets &after, TableMerge::Result *result)
{
    for (auto it = before.cbegin(); it != before.cend(); ++it) {
        result->removed[it.key()] = (it.value() - after.value(it.key())).size();
    }
    for (auto it = newer.cbegin(); it != newer.cend(); ++it) {
        result->edited[it.key()] = (QSet<QString>(it.value()) & after.value(it.key())).size();
    }
}

// One number from a query.
bool countRows(QSqlQuery &q, const QString &sql, int *count, QString *error)
{
    if (!q.exec(sql) || !q.next()) {
        return failed(q, error);
    }
    *count = q.value(0).toInt();
    q.finish();
    return true;
}

// What replacing would do, before any merge step: the rows the other holds, and the rows here it
// does not hold in this version or a newer one (a log's row is gone; an editable one gone or set back).
bool compare(QSqlDatabase &db, const QList<TableMerge::Table> &tables, TableMerge::Result *result, QString *error)
{
    QSqlQuery q(db);
    for (const TableMerge::Table &t : tables) {
        const bool log = t.kind == TableMerge::Kind::Log;
        const QString uid = t.columns.first();
        const QString held = QStringLiteral("SELECT COUNT(*) FROM %1.%2%3")
                                 .arg(kOther, t.name, log ? QString() : QStringLiteral(" WHERE deleted = 0"));
        const QString lost = QStringLiteral("SELECT COUNT(*) FROM main.%1 m WHERE %2NOT EXISTS (SELECT 1 FROM %3.%1 o"
                                            " WHERE o.%4 = m.%4%5)")
                                 .arg(t.name, log ? QString() : QStringLiteral("m.deleted = 0 AND "), kOther, uid,
                                      log ? QString() : QStringLiteral(" AND o.deleted = 0 AND o.changed >= m.changed"));
        if (!countRows(q, held, &result->holds[t.name], error) || !countRows(q, lost, &result->lost[t.name], error)) {
            return false;
        }
    }
    bool ok = true;
    result->theirMedia = liveMedia(db, kOther, tables, &ok, error).values();
    return ok;
}

bool attach(QSqlDatabase &db, const QString &path, const QString &name, QString *error)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("ATTACH DATABASE ? AS %1").arg(name));
    q.addBindValue(path);
    return q.exec() || failed(q, error);
}

// The tables of one copy from the other file, in its order (rows of one time stay in order).
bool takeRows(QSqlDatabase &db, const QString &here, const QString &there, const QList<TableMerge::Table> &tables,
              QString *error)
{
    QSqlQuery q(db);
    for (const TableMerge::Table &t : tables) {
        const QString all = t.columns.join(u", ");
        if (!q.exec(QStringLiteral("DELETE FROM %1.%2").arg(here, t.name))
            || !q.exec(QStringLiteral("INSERT INTO %1.%2 (%3) SELECT %3 FROM %4.%2 ORDER BY rowid")
                           .arg(here, t.name, all, there))) {
            return failed(q, error);
        }
    }
    return true;
}

// The first copy's file is the connection's main (the journal over all files needs a file there).
bool replaceAll(QSqlDatabase &db, const QList<TableMerge::Copy> &copies, QString *error)
{
    const auto here = [](qsizetype i) { return i == 0 ? kMain : QStringLiteral("here%1").arg(i); };
    const auto there = [](qsizetype i) { return kOther + QString::number(i); };
    for (qsizetype i = 0; i < copies.size(); ++i) {
        if ((i > 0 && !attach(db, copies.at(i).path, here(i), error)) || !attach(db, copies.at(i).other, there(i), error)) {
            return false;
        }
    }

    if (!db.transaction()) {
        *error = db.lastError().text();
        return false;
    }
    bool ok = true;
    for (qsizetype i = 0; ok && i < copies.size(); ++i) {
        ok = takeRows(db, here(i), there(i), copies.at(i).tables, error);
    }
    if (ok && !db.commit()) {
        *error = db.lastError().text();
        ok = false;
    }
    if (!ok) {
        db.rollback();
    }
    return ok;
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
    const QSet<QString> before = ok ? liveMedia(db, kMain, tables, &ok, error) : QSet<QString>();
    ok = ok && (mode != Mode::Preview || compare(db, tables, result, error));
    result->mine = before.values();
    const RowSets rowsBefore = ok && mode == Mode::Preview ? liveRows(db, tables, &ok, error) : RowSets();
    const RowSets rowsNewer = ok && mode == Mode::Preview ? newerRows(db, tables, &ok, error) : RowSets();
    ok = ok && hold(db, watch, error);
    for (const Table &t : tables) {
        ok = ok && mergeTable(db, t, watch, result, error);
    }
    ok = ok && (!after || after(db, error));
    if (ok && mode == Mode::Preview) {
        countOutcome(rowsBefore, rowsNewer, liveRows(db, tables, &ok, error), result);
    }

    // The media the rows name after the merge: only those come from the backup (a deleted note's
    // photo not); what they named before and no more goes.
    const QSet<QString> named = ok ? liveMedia(db, kMain, tables, &ok, error) : QSet<QString>();
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

bool TableMerge::replace(const QList<Copy> &copies, QString *error)
{
    const QString name = QStringLiteral("tablemerge-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        db.setDatabaseName(copies.first().path);
        ok = db.open();
        if (!ok) {
            *error = db.lastError().text();
        }
        ok = ok && replaceAll(db, copies, error);
        db.close();
    }
    QSqlDatabase::removeDatabase(name);
    return ok;
}
