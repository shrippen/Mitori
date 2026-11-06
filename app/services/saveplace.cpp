#include "saveplace.h"

#include <QFileInfo>

#include "typedpath.h"

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
    return TypedPath::resolve(picked, folder);
}

bool SavePlace::taken(const QUrl &target) const
{
    return asksItself() && target.isLocalFile() && QFileInfo::exists(target.toLocalFile());
}
