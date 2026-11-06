#include "syncaccount.h"

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
const QString kStoreKey = QStringLiteral("webdav-app-password");

QString iso(const QDateTime &t)
{
    return t.toUTC().toString(Qt::ISODateWithMs);
}

QDateTime fromIso(const QJsonValue &v)
{
    return QDateTime::fromString(v.toString(), Qt::ISODateWithMs).toUTC();
}

} // namespace

SyncAccount::SyncAccount(const QString &folder, std::shared_ptr<SecretStore> store)
    : m_folder(folder)
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
    // For its owner only: the password may land in it (stored()).
    s->sync();
    QFile::setPermissions(s->fileName(), QFile::ReadOwner | QFile::WriteOwner);

    if (account.password.isEmpty() || account.password == m_password) {
        return;
    }
    m_password = account.password;
    setSecret(Secret::Asking);
    storeSecret();
}

void SyncAccount::loadSecret()
{
    const auto s = settings();

    // Not moved yet, or the store failed last time: usable now, whatever the store answers.
    const QString inFile = s->value(kPassword).toString();
    if (!inFile.isEmpty()) {
        m_password = inFile;
        setSecret(Secret::File);
        storeSecret();
        return;
    }

    if (!s->value(kInStore, false).toBool()) {
        setSecret(Secret::None);
        return;
    }

    setSecret(Secret::Asking);
    const quint64 asked = ++m_asked;
    m_store->read(kStoreKey, this, [this, asked](SecretStore::Result result, const QString &password) {
        if (asked == m_asked) {
            found(result, password);
        }
    });
}

void SyncAccount::found(SecretStore::Result result, const QString &password)
{
    if (result == SecretStore::Result::Failed) {
        setSecret(Secret::Unreadable);
        return;
    }
    if (result == SecretStore::Result::NotFound || password.isEmpty()) {
        settings()->remove(kInStore);
        setSecret(Secret::None);
        return;
    }
    m_password = password;
    setSecret(Secret::KeyStore);
}

void SyncAccount::storeSecret()
{
    const quint64 asked = ++m_asked;
    m_store->write(kStoreKey, m_password, this, [this, asked](SecretStore::Result result) {
        if (asked == m_asked) {
            stored(result);
        }
    });
}

void SyncAccount::stored(SecretStore::Result result)
{
    const auto s = settings();

    // Confirmed: out of the file. If that cannot be written it stays there, and says so.
    if (result == SecretStore::Result::Ok) {
        s->remove(kPassword);
        s->setValue(kInStore, true);
        s->sync();
        setSecret(s->status() == QSettings::NoError ? Secret::KeyStore : Secret::File);
        return;
    }

    s->setValue(kPassword, m_password);
    s->sync();
    QFile::setPermissions(s->fileName(), QFile::ReadOwner | QFile::WriteOwner);
    setSecret(Secret::File);
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
