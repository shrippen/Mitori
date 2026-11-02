#include "bundles.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QSettings>
#include <QtConcurrent/QtConcurrentRun>

namespace {

// The public manifest (tools/bundle.py hochladen): which bundles exist, their files, the codes'
// entries. It holds no code and no key.
const QUrl kManifest(QStringLiteral("https://git.arianw.de/api/packages/shrippen/generic/mitori-manifest/1/manifest.json"));
constexpr int kFormat = 1;                      // "format" of the manifest
constexpr auto kWifiOnlyKey = "videos/wifiOnly";
constexpr qint64 kFreshSeconds = 6 * 60 * 60;
constexpr int kTimeoutMs = 30 * 1000;           // without a byte for this long a download fails
constexpr qint64 kProgressMs = 250;             // the least time between two progress reports
const QString kManifestFile = QStringLiteral("manifest.json");

// The file a video gets on the device: "aikido-k5.mvb" → "aikido-k5.mkv".
QString plainName(const QString &file)
{
    return QFileInfo(file).completeBaseName() + QStringLiteral(".mkv");
}

} // namespace

// MITORI_BUNDLES_MANIFEST: another manifest (tests: a local file).
Bundles::Bundles(QObject *parent)
    : QObject(parent)
    , m_shelf(&BundleShelf::shared())
    , m_source(qEnvironmentVariableIsSet("MITORI_BUNDLES_MANIFEST") ? QUrl(qEnvironmentVariable("MITORI_BUNDLES_MANIFEST")) : kManifest)
{
    loadCache();
}

Bundles::Bundles(const QUrl &manifest, const QString &root, QObject *parent)
    : QObject(parent)
    , m_ownShelf(std::make_unique<BundleShelf>(root))
    , m_shelf(m_ownShelf.get())
    , m_source(manifest)
{
    loadCache();
}

Bundles::~Bundles()
{
    for (const auto &job : std::as_const(m_jobs)) {
        if (job->reply) {
            job->reply->abort();
        }
    }
}

QNetworkAccessManager *Bundles::network()
{
    if (!m_network) {
        m_network = new QNetworkAccessManager(this);
    }
    return m_network;
}

bool Bundles::wifiOnly() const
{
    return QSettings().value(QLatin1String(kWifiOnlyKey), true).toBool();
}

void Bundles::setWifiOnly(bool on)
{
    if (on == wifiOnly()) {
        return;
    }
    QSettings().setValue(QLatin1String(kWifiOnlyKey), on);
    emit changed();
}

bool Bundles::onWifi()
{
    if (!QNetworkInformation::instance()
        && !QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::TransportMedium)) {
        return true;    // unknown: no way to tell, as on most desktops
    }
    const QNetworkInformation *info = QNetworkInformation::instance();
    if (info->supports(QNetworkInformation::Feature::Metered) && info->isMetered()) {
        return false;
    }
    switch (info->transportMedium()) {
    case QNetworkInformation::TransportMedium::Cellular:
        return false;
    default:
        return true;
    }
}

// The manifest as last loaded: the list and the codes' entries also offline.
void Bundles::loadCache()
{
    QFile file(QDir(m_shelf->root()).filePath(kManifestFile));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonObject m = QJsonDocument::fromJson(file.readAll()).object();
    if (m.value(QStringLiteral("format")).toInt() != kFormat) {
        return;
    }
    m_manifest = m;
    m_loaded = QFileInfo(file).lastModified(QTimeZone::UTC);
    m_manifestState = ManifestState::Ready;
}

void Bundles::reload(Reload when)
{
    if (m_manifestState == ManifestState::Loading) {
        return;
    }
    if (when == Reload::Older && m_loaded.isValid() && m_loaded.secsTo(QDateTime::currentDateTimeUtc()) < kFreshSeconds) {
        return;
    }
    QNetworkRequest request(m_source);
    request.setTransferTimeout(kTimeoutMs);
    QNetworkReply *reply = network()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] { received(reply); });
    m_manifestState = ManifestState::Loading;
    emit changed();
}

// Another format or no JSON (an error page) does not replace the manifest on the device.
void Bundles::received(QNetworkReply *reply)
{
    reply->deleteLater();
    const QByteArray data = reply->error() == QNetworkReply::NoError ? reply->readAll() : QByteArray();
    const QJsonObject m = QJsonDocument::fromJson(data).object();
    if (m.value(QStringLiteral("format")).toInt() != kFormat) {
        qWarning("Bundles: the manifest could not be loaded: %s", qPrintable(reply->errorString()));
        m_manifestState = ManifestState::Failed;
        emit changed();
        return;
    }
    QDir().mkpath(m_shelf->root());
    QSaveFile file(QDir(m_shelf->root()).filePath(kManifestFile));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.commit();
    }
    m_manifest = m;
    m_loaded = QDateTime::currentDateTimeUtc();
    m_manifestState = ManifestState::Ready;
    emit changed();
}

// Only bundles whose id can name a folder (BundleShelf::validId): the id goes into paths.
QJsonObject Bundles::manifestBundle(const QString &id) const
{
    if (!BundleShelf::validId(id)) {
        return {};
    }
    for (const QJsonValue &b : m_manifest.value(QStringLiteral("bundles")).toArray()) {
        if (b.toObject().value(QStringLiteral("id")).toString() == id) {
            return b.toObject();
        }
    }
    return {};
}

QVariantList Bundles::bundles() const
{
    QVariantList result;
    for (const QJsonValue &v : m_manifest.value(QStringLiteral("bundles")).toArray()) {
        const QJsonObject b = v.toObject();
        const QString id = b.value(QStringLiteral("id")).toString();
        if (!BundleShelf::validId(id)) {
            continue;
        }
        const int version = b.value(QStringLiteral("version")).toInt();
        const BundleShelf::Bundle shelf = m_shelf->bundle(id);
        qint64 size = 0;
        QStringList videos;
        for (const QJsonValue &f : b.value(QStringLiteral("videos")).toArray()) {
            size += qint64(f.toObject().value(QStringLiteral("groesse")).toDouble());
            videos << f.toObject().value(QStringLiteral("video")).toString();
        }

        State state = shelf.key.isEmpty() ? State::Locked
                    : shelf.version == 0  ? State::Unlocked
                    : shelf.version < version ? State::Update
                                              : State::Installed;
        if (m_failed.contains(id)) {
            state = m_failed.value(id);
        }
        double progress = 0;
        if (const auto job = m_jobs.value(id)) {
            state = State::Downloading;
            progress = job->total > 0 ? double(job->done + job->received) / double(job->total) : 0;
        }
        result << QVariantMap{{QStringLiteral("id"), id},
                              {QStringLiteral("uid"), id},         // the key of the page's rows (KeyedListModel)
                              {QStringLiteral("title"), b.value(QStringLiteral("titel")).toString()},
                              {QStringLiteral("version"), version},
                              {QStringLiteral("installed"), shelf.version},
                              {QStringLiteral("videos"), videos},
                              {QStringLiteral("size"), size},
                              {QStringLiteral("state"), QVariant::fromValue(state)},
                              {QStringLiteral("progress"), progress},
                              {QStringLiteral("hint"), b.value(QStringLiteral("hinweis")).toString()},
                              {QStringLiteral("code"), shelf.code.isEmpty() ? QString() : BundleCrypto::grouped(shelf.code)}};
    }
    return result;
}

int Bundles::updates() const
{
    int n = 0;
    for (const QVariant &b : bundles()) {
        n += b.toMap().value(QStringLiteral("state")).value<State>() == State::Update;
    }
    return n;
}

bool Bundles::codeValid(const QString &code) const
{
    return !BundleCrypto::normalized(code).isEmpty();
}

QString Bundles::grouped(const QString &code) const
{
    const QString n = BundleCrypto::normalized(code);
    return n.isEmpty() ? code : BundleCrypto::grouped(n);
}

void Bundles::unlock(const QString &input)
{
    const QString code = BundleCrypto::normalized(input);
    // A whole code whose check character does not fit has a typo; anything else is no code.
    if (code.isEmpty()) {
        emit unlockFailed(BundleCrypto::complete(input) ? UnlockError::Typo : UnlockError::Invalid);
        return;
    }
    if (m_manifest.isEmpty()) {
        emit unlockFailed(UnlockError::NoManifest);
        return;
    }
    const QJsonObject entry = m_manifest.value(QStringLiteral("codes")).toObject().value(BundleCrypto::lookupId(code)).toObject();
    const QString id = entry.value(QStringLiteral("bundle")).toString();
    if (entry.isEmpty() || manifestBundle(id).isEmpty()) {
        emit unlockFailed(UnlockError::Unknown);
        return;
    }

    // Argon2id takes a moment and 64 MiB: not on the thread that draws.
    const QByteArray salt = QByteArray::fromBase64(entry.value(QStringLiteral("salz")).toString().toLatin1());
    const QByteArray wrapped = QByteArray::fromBase64(entry.value(QStringLiteral("schluessel")).toString().toLatin1());
    const int passes = entry.value(QStringLiteral("t")).toInt();
    const int kibibytes = entry.value(QStringLiteral("m")).toInt();
    auto *watcher = new QFutureWatcher<QByteArray>(this);
    connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [this, watcher, code, id] {
        watcher->deleteLater();
        m_unlocking = false;
        const QByteArray key = watcher->result();
        if (key.isEmpty()) {
            emit changed();
            emit unlockFailed(UnlockError::Wrong);
            return;
        }
        if (!m_shelf->unlock(id, code, key)) {
            m_error = tr("The code could not be saved on this device.");
        }
        emit changed();
        emit unlocked(id);
    });
    m_unlocking = true;
    emit changed();
    watcher->setFuture(QtConcurrent::run(&BundleCrypto::unwrapKey, code, id, salt, passes, kibibytes, wrapped));
}

void Bundles::load(const QString &id, Network network)
{
    const QJsonObject b = manifestBundle(id);
    const BundleShelf::Bundle shelf = m_shelf->bundle(id);
    if (b.isEmpty() || shelf.key.isEmpty() || m_jobs.contains(id)) {
        return;
    }
    if (network == Network::WifiOnly && wifiOnly() && !onWifi()) {
        m_failed.insert(id, State::Waiting);
        emit changed();
        return;
    }
    m_failed.remove(id);
    m_error.clear();

    auto job = std::make_shared<Job>();
    job->id = id;
    job->version = b.value(QStringLiteral("version")).toInt();
    job->videos = b.value(QStringLiteral("videos")).toArray();
    for (const QJsonValue &f : std::as_const(job->videos)) {
        job->total += qint64(f.toObject().value(QStringLiteral("groesse")).toDouble());
    }
    // A fresh folder next to the bundle's: the loaded version stays until the new one is complete.
    QDir(m_shelf->folder(id) + QStringLiteral(".part")).removeRecursively();
    QDir().mkpath(m_shelf->folder(id) + QStringLiteral(".part"));
    m_jobs.insert(id, job);
    emit changed();
    next(*job);
}

void Bundles::next(Job &job)
{
    const QString id = job.id;
    const QString partDir = m_shelf->folder(id) + QStringLiteral(".part");

    // Every file in: the new version replaces the old one.
    if (job.videos.isEmpty()) {
        QDir(m_shelf->folder(id)).removeRecursively();
        if (!QDir().rename(partDir, m_shelf->folder(id))
            || !m_shelf->installed(id, job.version, job.files, job.cropped)) {
            fail(id, tr("The videos could not be saved on this device."));
            return;
        }
        m_jobs.remove(id);
        emit changed();
        emit installed(id);
        return;
    }

    const QJsonObject f = job.videos.first().toObject();
    const QString file = f.value(QStringLiteral("datei")).toString();
    const qint64 size = qint64(f.value(QStringLiteral("groesse")).toDouble());
    // Names from the manifest go into a path: only a plain file name.
    if (file.isEmpty() || file.contains(QLatin1Char('/')) || file.contains(QLatin1Char('\\')) || file.startsWith(QLatin1Char('.'))) {
        fail(id, tr("The manifest names a file that cannot be loaded."));
        return;
    }
    job.part = std::make_unique<QFile>(QDir(partDir).filePath(plainName(file)));
    if (!job.part->open(QIODevice::WriteOnly)) {
        fail(id, tr("The videos could not be saved on this device."));
        return;
    }
    job.decryptor = std::make_unique<BundleCrypto::Decryptor>(m_shelf->bundle(id).key, size, job.part.get());
    job.sha.reset();
    job.received = 0;

    const QUrl url(m_manifest.value(QStringLiteral("basis")).toString() + QStringLiteral("/mitori-") + id + QLatin1Char('/')
                   + QString::number(job.version) + QLatin1Char('/') + file);
    QNetworkRequest request(url);
    request.setTransferTimeout(kTimeoutMs);
    QNetworkReply *reply = network()->get(request);
    job.reply = reply;
    connect(reply, &QNetworkReply::readyRead, this, [this, id, reply] {
        const auto j = m_jobs.value(id);
        if (!j || j->reply != reply) {
            return;
        }
        const QByteArray data = reply->readAll();
        j->sha.addData(data);
        j->received += data.size();
        if (!j->decryptor->feed(data)) {
            reply->abort();
            return;
        }
        // Progress a few times a second: each change draws the page anew.
        if (j->shown.isValid() && j->shown.elapsed() < kProgressMs) {
            return;
        }
        j->shown.start();
        emit changed();
    });
    connect(reply, &QNetworkReply::finished, this, [this, id, reply, f] {
        reply->deleteLater();
        const auto j = m_jobs.value(id);
        if (!j || j->reply != reply) {
            return;
        }
        const QByteArray rest = reply->isOpen() ? reply->readAll() : QByteArray();
        j->sha.addData(rest);
        j->received += rest.size();
        const bool fed = j->decryptor->feed(rest);
        const bool ok = reply->error() == QNetworkReply::NoError && fed && j->decryptor->finish()
                        && QString::fromLatin1(j->sha.result().toHex()) == f.value(QStringLiteral("sha256")).toString();
        j->part->close();
        if (!ok) {
            fail(id, reply->error() != QNetworkReply::NoError && reply->error() != QNetworkReply::OperationCanceledError
                         ? tr("Loading failed: %1").arg(reply->errorString())
                         : tr("A video arrived damaged; load it again."));
            return;
        }
        const QString video = f.value(QStringLiteral("video")).toString();
        j->files.insert(video, QFileInfo(*j->part).fileName());
        if (f.value(QStringLiteral("beschnitten")).toBool()) {
            j->cropped << video;
        }
        j->done += qint64(f.value(QStringLiteral("groesse")).toDouble());
        j->received = 0;
        j->videos.removeFirst();
        next(*j);
    });
}

// The download stops; what was loaded before stays.
void Bundles::fail(const QString &id, const QString &why)
{
    if (const auto job = m_jobs.take(id)) {
        if (job->reply) {
            job->reply->abort();
        }
        if (job->part) {
            job->part->close();
        }
    }
    QDir(m_shelf->folder(id) + QStringLiteral(".part")).removeRecursively();
    qWarning("Bundles: %s: %s", qPrintable(id), qPrintable(why));
    m_error = why;
    m_failed.insert(id, State::Failed);
    emit changed();
}

void Bundles::cancel(const QString &id)
{
    if (!BundleShelf::validId(id)) {
        return;
    }
    const auto job = m_jobs.take(id);
    if (job && job->reply) {
        job->reply->abort();
    }
    QDir(m_shelf->folder(id) + QStringLiteral(".part")).removeRecursively();
    m_failed.remove(id);
    emit changed();
}

bool Bundles::remove(const QString &id)
{
    if (!BundleShelf::validId(id)) {
        return false;
    }
    cancel(id);
    const bool ok = m_shelf->remove(id);
    emit changed();
    return ok;
}
