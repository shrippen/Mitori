#include "devicelevels.h"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#elif defined(Q_OS_LINUX)
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QFile>
#include <QDBusReply>
#endif

namespace {

// MITORI_DEVICE_LEVELS: "memory" both levels in memory, "memory-screen" only the brightness
// (a device without a volume of its own, as the desktop).
bool inMemory()
{
    return qEnvironmentVariable("MITORI_DEVICE_LEVELS").startsWith(QLatin1String("memory"));
}

bool memoryVolume()
{
    return qEnvironmentVariable("MITORI_DEVICE_LEVELS") == QLatin1String("memory");
}

// The test's levels (MITORI_DEVICE_LEVELS=memory).
double g_brightness = 0.5;
double g_volume = 0.5;
// The brightness before the first change, to restore (Linux; Android restores by itself).
double g_original = -1;
// Whether the screen is kept on; on Linux the screen saver's cookie for releasing it.
bool g_held = false;
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
uint g_cookie = 0;
QString g_request;                              // the Inhibit portal's request; closing it releases
constexpr uint kInhibitIdle = 8;                // org.freedesktop.portal.Inhibit: flag "idle"
#endif

double clamped(double level)
{
    return std::clamp(level, 0.0, 1.0);
}

#ifdef Q_OS_ANDROID
constexpr int kStreamMusic = 3;                 // AudioManager.STREAM_MUSIC
constexpr jint kKeepScreenOn = 128;             // WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
constexpr float kSystemBrightness = -1.0f;      // WindowManager.LayoutParams.BRIGHTNESS_OVERRIDE_NONE

QJniObject audioManager()
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid()) {
        return {};
    }
    return context.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;",
                                    QJniObject::fromString(QStringLiteral("audio")).object<jstring>());
}

// The window's brightness, set on Android's main thread (the window belongs to it).
void windowBrightness(float level)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([level] {
        const QJniObject activity = QNativeInterface::QAndroidApplication::context();
        const QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) {
            return;
        }
        QJniObject attributes = window.callObjectMethod("getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
        attributes.setField<jfloat>("screenBrightness", level);
        window.callMethod<void>("setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V", attributes.object());
    });
}

// The window's level, or the system's (0..255) while the window has none of its own.
double androidBrightness()
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const QJniObject resolver = context.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
    const jint system = QJniObject::callStaticMethod<jint>(
        "android/provider/Settings$System", "getInt", "(Landroid/content/ContentResolver;Ljava/lang/String;I)I",
        resolver.object(), QJniObject::fromString(QStringLiteral("screen_brightness")).object<jstring>(), jint(128));
    return system / 255.0;
}
float g_window = kSystemBrightness;
#elif defined(Q_OS_LINUX)
// PowerDevil's brightness of the built-in screen (laptops, Plasma Mobile); none on a desktop
// with external screens only.
QDBusInterface &solid()
{
    static QDBusInterface i(QStringLiteral("org.kde.Solid.PowerManagement"),
                            QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                            QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"));
    return i;
}

int solidMax()
{
    const QDBusReply<int> max = solid().call(QStringLiteral("brightnessMax"));
    return max.isValid() ? max.value() : 0;
}
#endif

} // namespace

namespace DeviceLevels {

double brightness()
{
    if (inMemory()) {
        return g_brightness;
    }
#ifdef Q_OS_ANDROID
    return g_window >= 0 ? g_window : androidBrightness();
#elif defined(Q_OS_LINUX)
    const int max = solidMax();
    const QDBusReply<int> now = solid().call(QStringLiteral("brightness"));
    return max > 0 && now.isValid() ? double(now.value()) / max : -1;
#else
    return -1;
#endif
}

bool setBrightness(double level)
{
    level = clamped(level);
    if (g_original < 0) {
        g_original = brightness();
    }
    if (inMemory()) {
        g_brightness = level;
        return true;
    }
#ifdef Q_OS_ANDROID
    // Not quite dark: a black screen would look switched off.
    g_window = float(std::max(level, 0.02));
    windowBrightness(g_window);
    return true;
#elif defined(Q_OS_LINUX)
    const int max = solidMax();
    if (max <= 0) {
        return false;
    }
    const int value = std::max(1, int(std::lround(level * max)));
    return solid().call(QStringLiteral("setBrightnessSilent"), value).type() != QDBusMessage::ErrorMessage;
#else
    return false;
#endif
}

void restoreBrightness()
{
    if (g_original < 0) {
        return;
    }
    const double original = g_original;
    g_original = -1;
    if (inMemory()) {
        g_brightness = original;
        return;
    }
#ifdef Q_OS_ANDROID
    g_window = kSystemBrightness;
    windowBrightness(kSystemBrightness);
#else
    setBrightness(original);
    g_original = -1;
#endif
}

double volume()
{
    if (inMemory()) {
        return memoryVolume() ? g_volume : -1;
    }
#ifdef Q_OS_ANDROID
    const QJniObject audio = audioManager();
    if (!audio.isValid()) {
        return -1;
    }
    const jint max = audio.callMethod<jint>("getStreamMaxVolume", "(I)I", kStreamMusic);
    const jint now = audio.callMethod<jint>("getStreamVolume", "(I)I", kStreamMusic);
    return max > 0 ? double(now) / max : -1;
#else
    return -1;
#endif
}

bool setVolume(double level)
{
    level = clamped(level);
    if (inMemory()) {
        g_volume = level;
        return memoryVolume();
    }
#ifdef Q_OS_ANDROID
    const QJniObject audio = audioManager();
    if (!audio.isValid()) {
        return false;
    }
    const jint max = audio.callMethod<jint>("getStreamMaxVolume", "(I)I", kStreamMusic);
    audio.callMethod<void>("setStreamVolume", "(III)V", kStreamMusic, jint(std::lround(level * max)), jint(0));
    return true;
#else
    return false;
#endif
}

void holdScreen(Hold hold)
{
    const bool keep = hold == Hold::Keep;
    if (keep == g_held) {
        return;
    }
    g_held = keep;
    if (inMemory()) {
        return;
    }
#ifdef Q_OS_ANDROID
    // The window belongs to Android's main thread.
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([keep] {
        const QJniObject activity = QNativeInterface::QAndroidApplication::context();
        const QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) {
            return;
        }
        window.callMethod<void>(keep ? "addFlags" : "clearFlags", "(I)V", kKeepScreenOn);
    });
#elif defined(Q_OS_LINUX)
    // Inside the Flatpak the Inhibit portal (its bus proxy lets only org.freedesktop.portal.*
    // through), elsewhere the screen saver itself, as video players do.
    QDBusInterface saver(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/org/freedesktop/ScreenSaver"),
                         QStringLiteral("org.freedesktop.ScreenSaver"));
    if (!keep) {
        if (!g_request.isEmpty()) {
            QDBusInterface(QStringLiteral("org.freedesktop.portal.Desktop"), g_request,
                           QStringLiteral("org.freedesktop.portal.Request"))
                .call(QStringLiteral("Close"));
        } else if (g_cookie != 0) {
            saver.call(QStringLiteral("UnInhibit"), g_cookie);
        }
        g_request.clear();
        g_cookie = 0;
        return;
    }
    if (!QFile::exists(QStringLiteral("/.flatpak-info"))) {
        const QDBusReply<uint> cookie = saver.call(QStringLiteral("Inhibit"), QStringLiteral("Mitori"),
                                                   QStringLiteral("Video"));
        g_cookie = cookie.isValid() ? cookie.value() : 0;
        return;
    }
    QDBusInterface portal(QStringLiteral("org.freedesktop.portal.Desktop"), QStringLiteral("/org/freedesktop/portal/desktop"),
                          QStringLiteral("org.freedesktop.portal.Inhibit"));
    const QDBusReply<QDBusObjectPath> request = portal.call(QStringLiteral("Inhibit"), QString(), kInhibitIdle,
                                                            QVariantMap{{QStringLiteral("reason"), QStringLiteral("Video")}});
    g_request = request.isValid() ? request.value().path() : QString();
#endif
}

bool screenHeld()
{
    return g_held;
}

}
