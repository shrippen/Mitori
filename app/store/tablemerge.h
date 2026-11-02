#pragma once

#include <QDateTime>
#include <QHash>
#include <QList>
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
//
// Watched (the sync): a row changed on both sides since the last sync, to different texts, is a
// conflict. It stays as it is here until the user chooses, and is held on later merges.
namespace TableMerge {

enum class Kind { Log, Editable };
// Preview runs the same steps and rolls them back: the counts say what Apply would do.
enum class Mode { Preview, Apply };

struct Table {
    QString name;
    QStringList columns;        // the first is the uid
    Kind kind = Kind::Log;
    QString media;              // the column with a media file name, or ""
    QString text;               // the column two devices may change at once (a conflict), or ""
    QString parent;             // the row's technique or notebook column, for a conflict
};

// A row changed on both sides: this copy's text and the other's.
struct Conflict {
    QString table;
    QString uid;
    QString parent;
    QString mine;
    QString theirs;
    QDateTime mineChanged;      // UTC
    QDateTime theirsChanged;
    QString theirDevice;        // its device (the sync puts the name the user gave it)
};

// When conflicts count: changes after `since` on both sides, and the rows still `held` (open
// conflicts of an earlier merge). Without `since`, none.
struct Watch {
    QDateTime since;
    QStringList held;
};

struct Result {
    QHash<QString, int> added;          // {table: rows}
    QHash<QString, int> changed;
    QStringList orphans;                // media files the rows named before and no more
    QStringList media;                  // media files the rows name after the merge (not deleted)
    QList<Conflict> conflicts;
};

// Attaches `otherPath` (a file of the same schema), merges, runs `after` in the same transaction
// (a store's own clean-up), then commits or rolls back. False with `error` on failure.
using After = bool (*)(QSqlDatabase &db, QString *error);
bool run(QSqlDatabase &db, const QString &otherPath, const QList<Table> &tables, Mode mode,
         Result *result, QString *error, After after = nullptr, const Watch &watch = {});

}
