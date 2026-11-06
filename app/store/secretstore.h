#pragma once

#include <QString>
#include <functional>
#include <memory>

class QObject;

// The system's key store, the only way to it (Secret Service, KWallet, Android Keystore through
// QtKeychain; nothing else sees that). It asks a daemon that may wait for the user to unlock it,
// so every answer comes later on the calling thread, never during the call, and never after
// `context` is gone.
class SecretStore
{
public:
    enum class Result { Ok, NotFound, Failed };     // Failed: none there, locked or refused

    using ReadDone = std::function<void(Result, const QString &)>;
    using WriteDone = std::function<void(Result)>;

    virtual ~SecretStore() = default;

    virtual void read(const QString &key, QObject *context, ReadDone done) = 0;
    virtual void write(const QString &key, const QString &secret, QObject *context, WriteDone done) = 0;

    // The real one. `folder`: where Android's encrypted copy lies (its key stays in the Keystore),
    // a folder Android's backup leaves out.
    static std::shared_ptr<SecretStore> system(const QString &folder);
};
