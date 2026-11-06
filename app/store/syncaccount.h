#pragma once

#include "secretstore.h"
#include "tablemerge.h"
#include "webdav.h"

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <memory>

class QSettings;

// The sync's own data, in a folder Android's backup leaves out (`sync/` in the app's data
// folder): the account, this device's sync id, when it last synced, the other devices seen, and
// the open conflicts.
//
//   sync/konto.ini        [konto] server, user, passwordInStore, name, enabled, auto
//                                 password only while the key store does not hold it
//                         [stand] id, last, devices (JSON)
//   sync/konflikte.json   [{table, uid, parent, mine, theirs, …}]
//   sync/basis.json       {uid: changed}  each row as last synced (TableMerge::Watch)
//
// The id is not the rows' device id (kept in the settings, which a device transfer copies): a
// copied phone and its original must not upload to the same file.
//
// The app password lies in the key store (SecretStore), which answers later. account() gives it
// from memory, so the UI thread never waits and the sync's thread never sees the store:
//
//   start   in the file   usable at once; moved (below)
//           marked        read from the store (Asking); a sync asked for meanwhile waits (Sync)
//           neither       no password: the store is not touched, no unlock prompt for non-users
//           sync off      the store is not touched; switched on, the same as a start
//   new                   moved, never written to the file unless that fails
//
//   move    write, read back, compare; only then out of the file and marked. "Ok" proves nothing:
//           KWallet reports it for a failed write, and an empty read. Anything else: in the file.
//
// A password in the file is the newest: each start tries to move it again. The mark stays until a
// new password replaces it: "not found" may be a refused unlock or another backend (Unreadable).
class SyncAccount : public QObject
{
    Q_OBJECT

public:
    struct Device {
        QString id;
        QString name;
        QDateTime synced;       // UTC: when it uploaded last
    };

    enum class Toggle { Off, On };

    // Where the app password is.
    enum class Secret {
        None,
        Unasked,        // marked, but sync is off: the store was not asked
        Asking,         // the store reads it, or takes a new one (in memory until it answers)
        KeyStore,
        Moving,         // in the file; the store did not answer yet (with sync off: was not asked)
        File,           // in the file: the store did not take it
        NotRemoved,     // the store holds it, but the file could not be rewritten: in both
        NotSaved,       // neither store nor file took it: in memory only
        Unreadable,     // marked, the store does not give it out
    };

    SyncAccount(const QString &folder, std::shared_ptr<SecretStore> store);
    static QString defaultFolder();

    // Finds the password (the table above), unless known or on its way; secretChanged() as its
    // place changes.
    void loadSecret();
    Secret secret() const { return m_secret; }

    // With the password as far as known: empty while the store is read and when Unreadable.
    WebDav::Account account() const;
    void setAccount(const WebDav::Account &account);
    QString name() const;
    void setName(const QString &name);
    bool enabled() const;
    void setEnabled(Toggle on);
    bool automatic() const;
    void setAutomatic(Toggle on);

    // This device's sync id (made once).
    QString id();
    QDateTime last() const;
    void setLast(const QDateTime &at);
    QList<Device> devices() const;
    void setDevices(const QList<Device> &devices);

    // Each note's and entry's `changed` at the last sync.
    QHash<QString, QDateTime> base() const;
    bool setBase(const QHash<QString, QDateTime> &base);

    QList<TableMerge::Conflict> conflicts() const;
    bool setConflicts(const QList<TableMerge::Conflict> &conflicts);

signals:
    void secretChanged();

private:
    // The account file (and its folder, made on first use).
    std::unique_ptr<QSettings> settings() const;

    void setSecret(Secret secret);
    void storeSecret();
    void checkStored();
    void commitStored();
    void keepInFile();
    void found(SecretStore::Result result, const QString &password);

    QString m_folder;
    QString m_storeKey;         // of this data folder: profiles and test runs keep theirs
    std::shared_ptr<SecretStore> m_store;
    QString m_password;
    Secret m_secret = Secret::None;
    quint64 m_asked = 0;        // the latest question to the store: a newer password voids older answers
};
