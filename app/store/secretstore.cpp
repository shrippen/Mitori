#include "secretstore.h"

#include <QDebug>
#include <QDir>
#include <QObject>
#include <QSettings>
#include <qt6keychain/keychain.h>

namespace {

const QString kService = QStringLiteral("Mitori");
const QString kEncryptedFile = QStringLiteral("schluessel.ini");

SecretStore::Result resultOf(const QKeychain::Job *job)
{
    if (job->error() == QKeychain::NoError) {
        return SecretStore::Result::Ok;
    }
    if (job->error() == QKeychain::EntryNotFound) {
        return SecretStore::Result::NotFound;
    }
    qWarning() << "Key store:" << job->errorString();
    return SecretStore::Result::Failed;
}

class KeychainStore final : public SecretStore
{
public:
    explicit KeychainStore(const QString &folder)
        : m_file(QDir(folder).filePath(kEncryptedFile))
    {
    }

    void read(const QString &key, QObject *context, ReadDone done) override
    {
        auto *job = new QKeychain::ReadPasswordJob(kService);
        prepare(job, key);
        QObject::connect(job, &QKeychain::Job::finished, context, [job, done = std::move(done)] {
            done(resultOf(job), job->textData());
        });
        job->start();
    }

    void write(const QString &key, const QString &secret, QObject *context, WriteDone done) override
    {
        auto *job = new QKeychain::WritePasswordJob(kService);
        prepare(job, key);
        job->setTextData(secret);
        QObject::connect(job, &QKeychain::Job::finished, context, [job, done = std::move(done)] {
            done(resultOf(job));
        });
        job->start();
    }

private:
    // No parent: libsecret's callback holds the job, so it lives until it answers (and deletes
    // itself). No insecure fallback: the file is SyncAccount's decision, and visible.
    void prepare(QKeychain::Job *job, const QString &key) const
    {
        job->setKey(key);
        job->setSettings(new QSettings(m_file, QSettings::IniFormat, job));
    }

    QString m_file;
};

} // namespace

std::shared_ptr<SecretStore> SecretStore::system(const QString &folder)
{
    return std::make_shared<KeychainStore>(folder);
}
