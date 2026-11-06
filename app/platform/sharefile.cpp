#include "sharefile.h"

#include <QStandardPaths>

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#endif

namespace {

constexpr auto kFolder = "/export";     // <cache-path> in res/xml/share_paths.xml

#ifdef Q_OS_ANDROID
constexpr auto kAuthority = ".fileprovider";    // the provider's authorities in the manifest
constexpr auto kMimeType = "application/zip";

QJniObject jstr(const QString &text)
{
    return QJniObject::fromString(text);
}

// file ──FileProvider──► content:// ──ACTION_SEND──► chooser ──► the app the user picks. The grant
// goes with the intent: startActivity moves EXTRA_STREAM to the chooser's ClipData.
bool sendOnAndroid(const QString &path, const QString &subject)
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid()) {
        return false;
    }

    QJniEnvironment env;
    const QString package = context.callObjectMethod("getPackageName", "()Ljava/lang/String;").toString();
    const QJniObject file("java/io/File", "(Ljava/lang/String;)V", jstr(path).object<jstring>());
    const QJniObject uri = QJniObject::callStaticObjectMethod(
        "androidx/core/content/FileProvider", "getUriForFile",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;",
        context.object(), jstr(package + QLatin1String(kAuthority)).object<jstring>(), file.object());
    if (env.checkAndClearExceptions() || !uri.isValid()) {
        return false;       // a path the provider does not serve (IllegalArgumentException)
    }

    QJniObject intent("android/content/Intent", "(Ljava/lang/String;)V",
                      jstr(QStringLiteral("android.intent.action.SEND")).object<jstring>());
    intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;",
                            jstr(QLatin1String(kMimeType)).object<jstring>());
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;Landroid/os/Parcelable;)Landroid/content/Intent;",
                            jstr(QStringLiteral("android.intent.extra.STREAM")).object<jstring>(), uri.object());
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                            jstr(QStringLiteral("android.intent.extra.SUBJECT")).object<jstring>(),
                            jstr(subject).object<jstring>());
    constexpr jint kGrantRead = 0x00000001;     // Intent.FLAG_GRANT_READ_URI_PERMISSION
    intent.callObjectMethod("addFlags", "(I)Landroid/content/Intent;", kGrantRead);

    // Without a title: the system says "Share" in the device's language.
    QJniObject chooser = QJniObject::callStaticObjectMethod(
        "android/content/Intent", "createChooser",
        "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;",
        intent.object(), static_cast<jobject>(nullptr));
    constexpr jint kNewTask = 0x10000000;   // Intent.FLAG_ACTIVITY_NEW_TASK: the context may not be an activity
    chooser.callObjectMethod("addFlags", "(I)Landroid/content/Intent;", kNewTask);

    context.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", chooser.object());
    return !env.checkAndClearExceptions();
}
#endif

} // namespace

namespace ShareFile {

bool available()
{
#ifdef Q_OS_ANDROID
    return true;
#else
    return false;
#endif
}

QString folder()
{
#ifdef Q_OS_ANDROID
    // The provider's <cache-path> is context.getCacheDir(), so from there and not from Qt's mapping.
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const QJniObject cache = context.callObjectMethod("getCacheDir", "()Ljava/io/File;");
    return cache.callObjectMethod("getAbsolutePath", "()Ljava/lang/String;").toString() + QLatin1String(kFolder);
#else
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String(kFolder);
#endif
}

bool send(const QString &path, const QString &subject)
{
#ifdef Q_OS_ANDROID
    return sendOnAndroid(path, subject);
#else
    Q_UNUSED(path)
    Q_UNUSED(subject)
    return false;
#endif
}

}
