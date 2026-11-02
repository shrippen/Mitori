#include "syncaccount.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

using namespace Qt::StringLiterals;

namespace {

const QString kAccountFile = QStringLiteral("konto.ini");
const QString kConflictFile = QStringLiteral("konflikte.json");

QString iso(const QDateTime &t)
{
    return t.toUTC().toString(Qt::ISODateWithMs);
}

QDateTime fromIso(const QJsonValue &v)
{
    return QDateTime::fromString(v.toString(), Qt::ISODateWithMs).toUTC();
}

} // namespace

SyncAccount::SyncAccount(const QString &folder)
    : m_folder(folder)
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
            s->value(QStringLiteral("konto/password")).toString()};
}

void SyncAccount::setAccount(const WebDav::Account &account)
{
    const auto s = settings();
    s->setValue(QStringLiteral("konto/server"), account.folder.toString());
    s->setValue(QStringLiteral("konto/user"), account.user);
    s->setValue(QStringLiteral("konto/password"), account.password);
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
