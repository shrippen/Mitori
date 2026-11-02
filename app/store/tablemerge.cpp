#include "tablemerge.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSet>
#include <QSqlQuery>

namespace {

const QString kOther = QStringLiteral("other");

bool failed(const QSqlQuery &q, QString *error)
{
    *error = q.lastError().text();
    return false;
}

// One table: the rows this copy lacks, then (editable) the newer ones.
bool mergeTable(QSqlDatabase &db, const TableMerge::Table &t, TableMerge::Result *result, QString *error)
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

    const QString newer = QStringLiteral("o.%1 = main.%2.%1 AND o.changed > main.%2.changed").arg(uid, t.name);
    const QString rest = t.columns.mid(1).join(u", ");
    if (!q.exec(QStringLiteral("UPDATE main.%1 SET (%2) = (SELECT %2 FROM %3.%1 o WHERE o.%4 = main.%1.%4)"
                               " WHERE EXISTS (SELECT 1 FROM %3.%1 o WHERE %5)")
                    .arg(t.name, rest, kOther, uid, newer))) {
        return failed(q, error);
    }
    result->changed[t.name] = q.numRowsAffected();
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
                     Result *result, QString *error, After after)
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
    for (const Table &t : tables) {
        ok = ok && mergeTable(db, t, result, error);
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
