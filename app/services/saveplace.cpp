#include "saveplace.h"

#include <QFileInfo>

#include "typedpath.h"

namespace {
constexpr auto kSuffix = "zip";         // the dialog saves ZIP archives
}

bool SavePlace::asksItself() const
{
#ifdef Q_OS_ANDROID
    return false;
#else
    return !qEnvironmentVariableIsSet("FLATPAK_ID");
#endif
}

QUrl SavePlace::resolve(const QUrl &picked, const QUrl &folder) const
{
    // A portal's path is the granted file, Android's a content URL: nothing to read into them, and
    // another name would be written as a hidden file.
    if (!asksItself()) {
        return picked;
    }
    return TypedPath::resolve(picked, folder, kSuffix);
}

bool SavePlace::taken(const QUrl &target) const
{
    return asksItself() && target.isLocalFile() && QFileInfo(target.toLocalFile()).isFile();
}
