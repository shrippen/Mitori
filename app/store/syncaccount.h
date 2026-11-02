#pragma once

#include "tablemerge.h"
#include "webdav.h"

#include <QDateTime>
#include <QList>
#include <QString>
#include <memory>

class QSettings;

// The sync's own data, in a folder Android's backup leaves out (`sync/` in the app's data
// folder): the account with its app password, this device's sync id, when it last synced, the
// other devices seen, and the open conflicts.
//
//   sync/konto.ini        [konto] server, user, password, folder, name, enabled, auto
//                         [stand] id, last, devices (JSON)
//   sync/konflikte.json   [{table, uid, parent, mine, theirs, …}]
//
// The id is not the rows' device id (kept in the settings, which a device transfer copies): a
// copied phone and its original must not upload to the same file.
class SyncAccount
{
public:
    struct Device {
        QString id;
        QString name;
        QDateTime synced;       // UTC: when it uploaded last
    };

    enum class Toggle { Off, On };

    explicit SyncAccount(const QString &folder);
    static QString defaultFolder();

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

    QList<TableMerge::Conflict> conflicts() const;
    bool setConflicts(const QList<TableMerge::Conflict> &conflicts);

private:
    // The account file (and its folder, made on first use).
    std::unique_ptr<QSettings> settings() const;

    QString m_folder;
};
