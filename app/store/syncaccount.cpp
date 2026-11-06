#include "syncaccount.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTimeZone>
#include <QUuid>

using namespace Qt::StringLiterals;

namespace {

const QString kAccountFile = QStringLiteral("konto.ini");
const QString kConflictFile = QStringLiteral("konflikte.json");
const QString kBaseFile = QStringLiteral("basis.json");
const QString kPassword = QStringLiteral("konto/password");
const QString kInStore = QStringLiteral("konto/passwordInStore");
const QString kStoreKey = QStringLiteral("webdav-app-password-%1");
constexpr int kFolderHashLength = 12;

QString iso(const QDateTime &t)
{
    return t.toUTC().toString(Qt::ISODateWithMs);
}

// webdav-app-password-3f2a…, the same for the same folder.
QString storeKey(const QString &folder)
{
    const QByteArray path = QDir(folder).absolutePath().toUtf8();
    return kStoreKey.arg(QString::fromLatin1(QCryptographicHash::hash(path, QCryptographicHash::Sha256).toHex().left(kFolderHashLength)));
}

QDateTime fromIso(const QJsonValue &v)
{
    return QDateTime::fromString(v.toString(), Qt::ISODateWithMs).toUTC();
}

} // namespace

SyncAccount::SyncAccount(const QString &folder, std::shared_ptr<SecretStore> store)
    : m_folder(folder)
    , m_storeKey(storeKey(folder))
    , m_store(std::move(store))
{
}

QString SyncAccount::defaultFolder()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/sync");
}

std::unique_ptr<QSettings> SyncAccount::settings() const
{
    QDir().mkpath(m_folder);
    return std::make_unique<QSettings>(QDir(m_folder).filePath(kAccountFile), QSettings::IniFormat);
}

WebDav::Account SyncAccount::account() const
{
    const auto s = settings();
    return {QUrl(s->value(QStringLiteral("konto/server")).toString()), s->value(QStringLiteral("konto/user")).toString(),
            m_password};
}

void SyncAccount::setAccount(const WebDav::Account &account)
{
    const auto s = settings();
    s->setValue(QStringLiteral("konto/server"), account.folder.toString());
    s->setValue(QStringLiteral("konto/user"), account.user);
    // For its owner only: the password may land in it (keepInFile()).
    s->sync();
    QFile::setPermissions(s->fileName(), QFile::ReadOwner | QFile::WriteOwner);

    // The same one again counts when it is not on record: that is how the note repairs it.
    const bool onRecord = m_secret != Secret::NotRecorded && m_secret != Secret::NotSaved;
    if (account.password.isEmpty() || (account.password == m_password && onRecord)) {
        return;
    }
    m_password = account.password;
    setSecret(Secret::Asking);
    storeSecret();
}

bool SyncAccount::passwordSaved() const
{
    switch (m_secret) {
    case Secret::Unasked:
    case Secret::KeyStore:
    case Secret::Moving:
    case Secret::File:
    case Secret::NotRemoved:
        return true;
    case Secret::Asking:
        return m_password.isEmpty();    // reading a saved one, not holding a new one
    default:
        return false;
    }
}

void SyncAccount::loadSecret()
{
    if (m_secret == Secret::Asking || m_secret == Secret::KeyStore) {
        return;
    }
    const auto s = settings();
    const bool on = enabled();

    // Not moved yet, or the store failed last time: usable now, whatever the store answers.
    const QString inFile = s->value(kPassword).toString();
    if (!inFile.isEmpty()) {
        m_password = inFile;
        setSecret(Secret::Moving);
        if (on) {
            storeSecret();
        }
        return;
    }

    if (!s->value(kInStore, false).toBool()) {
        setSecret(Secret::None);
        return;
    }
    if (!on) {
        setSecret(Secret::Unasked);
        return;
    }

    setSecret(Secret::Asking);
    const quint64 asked = ++m_asked;
    m_store->read(m_storeKey, this, [this, asked](SecretStore::Result result, const QString &password) {
        if (asked == m_asked) {
            found(result, password);
        }
    });
}

void SyncAccount::found(SecretStore::Result result, const QString &password)
{
    // Empty is no password, NotFound no proof that it is gone: the mark stays, the next start asks.
    if (result != SecretStore::Result::Ok || password.isEmpty()) {
        setSecret(Secret::Unreadable);
        return;
    }
    m_password = password;
    setSecret(Secret::KeyStore);
}

void SyncAccount::storeSecret()
{
    const quint64 asked = ++m_asked;
    m_store->write(m_storeKey, m_password, this, [this, asked](SecretStore::Result result) {
        if (asked != m_asked) {
            return;
        }
        if (result == SecretStore::Result::Ok) {
            checkStored();
        } else {
            keepInFile();
        }
    });
}

// Only what reads back counts as stored.
void SyncAccount::checkStored()
{
    const quint64 asked = ++m_asked;
    m_store->read(m_storeKey, this, [this, asked](SecretStore::Result result, const QString &password) {
        if (asked != m_asked) {
            return;
        }
        if (result == SecretStore::Result::Ok && password == m_password) {
            commitStored();
        } else {
            keepInFile();
        }
    });
}

void SyncAccount::commitStored()
{
    const auto s = settings();
    const QString inFile = s->value(kPassword).toString();
    const bool marked = s->value(kInStore, false).toBool();

    s->remove(kPassword);
    s->setValue(kInStore, true);
    s->sync();
    if (s->status() == QSettings::NoError) {
        setSecret(Secret::KeyStore);
        return;
    }

    // Not written: back to what the file says, so that what follows judges by that.
    restoreFile(*s, inFile);
    if (!marked) {
        s->remove(kInStore);
    }
    if (inFile == m_password) {
        setSecret(Secret::NotRemoved);
        return;
    }
    setSecret(inFile.isEmpty() && marked ? Secret::KeyStore : Secret::NotRecorded);
}

void SyncAccount::keepInFile()
{
    const auto s = settings();
    const QString inFile = s->value(kPassword).toString();

    // Already there: a file that cannot be written loses nothing.
    if (inFile == m_password) {
        setSecret(Secret::File);
        return;
    }

    s->setValue(kPassword, m_password);
    s->sync();
    if (s->status() != QSettings::NoError) {
        restoreFile(*s, inFile);
        setSecret(Secret::NotSaved);
        return;
    }
    QFile::setPermissions(s->fileName(), QFile::ReadOwner | QFile::WriteOwner);
    setSecret(Secret::File);
}

// QSettings keeps a failed write pending and serves it as if done.
void SyncAccount::restoreFile(QSettings &s, const QString &inFile)
{
    if (inFile.isEmpty()) {
        s.remove(kPassword);
    } else {
        s.setValue(kPassword, inFile);
    }
}

void SyncAccount::setSecret(Secret secret)
{
    if (m_secret != secret) {
        m_secret = secret;
        emit secretChanged();
    }
}

QString SyncAccount::name() const
{
    return settings()->value(QStringLiteral("konto/name")).toString();
}

void SyncAccount::setName(const QString &name)
{
    settings()->setValue(QStringLiteral("konto/name"), name.trimmed());
}

bool SyncAccount::enabled() const
{
    return settings()->value(QStringLiteral("konto/enabled"), false).toBool();
}

void SyncAccount::setEnabled(Toggle on)
{
    settings()->setValue(QStringLiteral("konto/enabled"), on == Toggle::On);
}

bool SyncAccount::automatic() const
{
    return settings()->value(QStringLiteral("konto/auto"), false).toBool();
}

void SyncAccount::setAutomatic(Toggle on)
{
    settings()->setValue(QStringLiteral("konto/auto"), on == Toggle::On);
}

QString SyncAccount::id()
{
    const auto s = settings();
    QString id = s->value(QStringLiteral("stand/id")).toString();
    if (id.isEmpty()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        s->setValue(QStringLiteral("stand/id"), id);
    }
    return id;
}

QDateTime SyncAccount::last() const
{
    return QDateTime::fromString(settings()->value(QStringLiteral("stand/last")).toString(), Qt::ISODateWithMs).toUTC();
}

void SyncAccount::setLast(const QDateTime &at)
{
    settings()->setValue(QStringLiteral("stand/last"), iso(at));
}

QList<SyncAccount::Device> SyncAccount::devices() const
{
    QList<Device> list;
    const QJsonArray array = QJsonDocument::fromJson(settings()->value(QStringLiteral("stand/devices")).toByteArray()).array();
    for (const QJsonValue &v : array) {
        const QJsonObject o = v.toObject();
        list << Device{o.value(u"id").toString(), o.value(u"name").toString(), fromIso(o.value(u"synced"))};
    }
    return list;
}

void SyncAccount::setDevices(const QList<Device> &devices)
{
    QJsonArray array;
    for (const Device &d : devices) {
        array << QJsonObject{{u"id"_s, d.id}, {u"name"_s, d.name}, {u"synced"_s, iso(d.synced)}};
    }
    settings()->setValue(QStringLiteral("stand/devices"), QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QList<TableMerge::Conflict> SyncAccount::conflicts() const
{
    QFile file(QDir(m_folder).filePath(kConflictFile));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QList<TableMerge::Conflict> list;
    for (const QJsonValue &v : QJsonDocument::fromJson(file.readAll()).array()) {
        const QJsonObject o = v.toObject();
        list << TableMerge::Conflict{o.value(u"table").toString(), o.value(u"uid").toString(),
                                     o.value(u"parent").toString(), o.value(u"mine").toString(),
                                     o.value(u"theirs").toString(), fromIso(o.value(u"mineChanged")),
                                     fromIso(o.value(u"theirsChanged")), o.value(u"theirDevice").toString()};
    }
    return list;
}

bool SyncAccount::setConflicts(const QList<TableMerge::Conflict> &conflicts)
{
    QJsonArray array;
    for (const TableMerge::Conflict &c : conflicts) {
        array << QJsonObject{{u"table"_s, c.table},        {u"uid"_s, c.uid},
                             {u"parent"_s, c.parent},      {u"mine"_s, c.mine},
                             {u"theirs"_s, c.theirs},      {u"mineChanged"_s, iso(c.mineChanged)},
                             {u"theirsChanged"_s, iso(c.theirsChanged)}, {u"theirDevice"_s, c.theirDevice}};
    }
    QDir().mkpath(m_folder);
    QSaveFile file(QDir(m_folder).filePath(kConflictFile));
    return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(array).toJson()) >= 0 && file.commit();
}

QHash<QString, QDateTime> SyncAccount::base() const
{
    QFile file(QDir(m_folder).filePath(kBaseFile));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QHash<QString, QDateTime> base;
    const QJsonObject o = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = o.constBegin(); it != o.constEnd(); ++it) {
        base.insert(it.key(), QDateTime::fromMSecsSinceEpoch(it.value().toInteger(), QTimeZone::UTC));
    }
    return base;
}

bool SyncAccount::setBase(const QHash<QString, QDateTime> &base)
{
    QJsonObject o;
    for (auto it = base.cbegin(); it != base.cend(); ++it) {
        o.insert(it.key(), it.value().toMSecsSinceEpoch());
    }
    QDir().mkpath(m_folder);
    QSaveFile file(QDir(m_folder).filePath(kBaseFile));
    return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(o).toJson(QJsonDocument::Compact)) >= 0
           && file.commit();
}
