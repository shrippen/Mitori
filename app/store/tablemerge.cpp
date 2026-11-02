#include "tablemerge.h"

#include <QSqlDatabase>
#include <QSqlError>
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

    // A newer row from the other copy: the media file it had here goes when the row drops it.
    const QString newer = QStringLiteral("o.%1 = main.%2.%1 AND o.changed > main.%2.changed").arg(uid, t.name);
    if (!t.media.isEmpty()) {
        if (!q.exec(QStringLiteral("SELECT main.%1.%2 FROM main.%1 JOIN %3.%1 o ON %4"
                                   " WHERE main.%1.%2 <> '' AND (o.%2 <> main.%1.%2 OR o.deleted = 1)")
                        .arg(t.name, t.media, kOther, newer))) {
            return failed(q, error);
        }
        while (q.next()) {
            result->orphans << q.value(0).toString();
        }
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
    for (const Table &t : tables) {
        ok = ok && mergeTable(db, t, result, error);
    }
    ok = ok && (!after || after(db, error));

    // The media the merged rows name: only those come from the backup (a deleted note's photo not).
    for (const Table &t : tables) {
        if (!ok || t.media.isEmpty()) {
            continue;
        }
        QSqlQuery m(db);
        ok = m.exec(QStringLiteral("SELECT %1 FROM main.%2 WHERE deleted = 0 AND %1 <> ''").arg(t.media, t.name));
        if (!ok) {
            *error = m.lastError().text();
        }
        while (m.next()) {
            result->media << m.value(0).toString();
        }
    }
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
