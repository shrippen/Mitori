#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class QSqlDatabase;

// Merges the tables of another copy of a store into this one (a backup read back): rows match by
// uid. A log (ratings, exam rounds) takes the rows it lacks; an editable table (notes, entries)
// also takes a row whose `changed` is newer, deletion marks included.
//
//   this    note a  changed 10:00        other  note a  changed 11:00   → a from other (changed)
//           note b  changed 12:00               note b  changed 09:00   → b stays
//                                               note c                  → c added
namespace TableMerge {

enum class Kind { Log, Editable };
// Preview runs the same steps and rolls them back: the counts say what Apply would do.
enum class Mode { Preview, Apply };

struct Table {
    QString name;
    QStringList columns;        // the first is the uid
    Kind kind = Kind::Log;
    QString media;              // the column with a media file name, or ""
};

struct Result {
    QHash<QString, int> added;          // {table: rows}
    QHash<QString, int> changed;
    QStringList orphans;                // media files the rows named before and no more
    QStringList media;                  // media files the rows name after the merge (not deleted)
};

// Attaches `otherPath` (a file of the same schema), merges, runs `after` in the same transaction
// (a store's own clean-up), then commits or rolls back. False with `error` on failure.
using After = bool (*)(QSqlDatabase &db, QString *error);
bool run(QSqlDatabase &db, const QString &otherPath, const QList<Table> &tables, Mode mode,
         Result *result, QString *error, After after = nullptr);

}
