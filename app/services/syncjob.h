#pragma once

#include "syncaccount.h"
#include "tablemerge.h"
#include "webdav.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>

// One sync with the WebDAV folder, start to end (plain C++, it runs on a thread of its own):
//
//   <folder>/geraete/<id>.zip   each device's data without media (BackupArchive)
//   <folder>/medien/<name>      every photo and voice note, once
//
//   1  folders made, both listed
//   2  every other device's archive merged in (TableMerge; a row changed on both sides since the
//      last sync is a conflict, held until the user chooses)
//   3  media the merged rows name fetched; media of this device the folder lacks sent
//   4  this device's archive sent
namespace SyncJob {

struct Input {
    WebDav::Account account;
    QString id;                 // this device's sync id
    QString name;               // and its name for the others
    QString rowsDevice;         // the rows' device id (manifest)
    QHash<QString, QDateTime> base;         // each note's and entry's `changed` at the last sync
    QList<TableMerge::Conflict> held;
    QString practice;           // the device's files
    QString courses;
    QString media;
    QString work;               // a folder for the downloads
};

struct Output {
    bool ok = false;
    QString error;
    QDateTime started;          // UTC
    QHash<QString, QDateTime> base;         // as uploaded: the next sync's base
    QList<SyncAccount::Device> devices;     // the other devices in the folder
    QList<TableMerge::Conflict> conflicts;  // open after this sync
    int received = 0;           // rows added or changed here
};

Output run(const Input &input);

}
