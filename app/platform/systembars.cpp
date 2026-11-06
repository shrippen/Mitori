#include "systembars.h"

#include <QColor>
#include <QtGlobal>

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>

namespace {
constexpr jint kDrawsBars = jint(0x80000000);     // LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS
constexpr jint kLightStatus = 0x2000;             // View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR
constexpr jint kLightNavigation = 0x10;           // View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR
constexpr double kLightGround = 0.5;

void paintOnUiThread(jint argb, bool light)
{
    const QJniObject activity = QNativeInterface::QAndroidApplication::context();
    const QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
    if (!window.isValid()) {
        return;
    }
    window.callMethod<void>("addFlags", "(I)V", kDrawsBars);
    window.callMethod<void>("setStatusBarColor", "(I)V", argb);
    window.callMethod<void>("setNavigationBarColor", "(I)V", argb);

    const QJniObject decor = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
    const jint flags = decor.callMethod<jint>("getSystemUiVisibility", "()I");
    const jint lightFlags = kLightStatus | kLightNavigation;
    decor.callMethod<void>("setSystemUiVisibility", "(I)V", light ? flags | lightFlags : flags & ~lightFlags);
}
} // namespace

void paintSystemBars(const QColor &ground)
{
    const jint argb = jint(ground.rgba());
    const bool light = ground.lightnessF() > kLightGround;
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([argb, light] { paintOnUiThread(argb, light); });
}
#else
void paintSystemBars(const QColor &ground)
{
    Q_UNUSED(ground)
}
#endif
