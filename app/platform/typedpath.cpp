#include "typedpath.h"

#include <QDir>
#include <QFileInfo>

namespace {

// The file the path means. Qt joins folder + "/" + text, and the folder may end in "/" itself
// (after Save with an empty name, or "sub/" and Save):
//
//   /docs   + /var/x.zip  →  /docs//var/x.zip    typed "/var/x.zip"
//   /docs/  + y.zip       →  /docs//y.zip        typed "y.zip"
QString meant(const QString &path, const QString &folder)
{
    const QString prefix = folder + QLatin1Char('/');
    const bool inFolder = path.startsWith(prefix);
    const QString typed = inFolder ? path.mid(prefix.size()) : path;

    if (typed.startsWith(QLatin1String("~/"))) {
        return QDir::homePath() + typed.mid(1);
    }
    if (typed.startsWith(QLatin1Char('/'))) {
        return typed;
    }

    QString base = folder;
    if (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }
    return base + QLatin1Char('/') + typed;
}

// A file gets the suffix unless it has it; a folder is left alone (no hidden ".zip" in it).
QString withSuffix(const QString &path, const QString &suffix)
{
    if (path.endsWith(QLatin1Char('/')) || QFileInfo(path).isDir()) {
        return path;
    }
    const QString ending = QLatin1Char('.') + suffix;
    return path.endsWith(ending, Qt::CaseInsensitive) ? path : path + ending;
}

} // namespace

QUrl TypedPath::resolve(const QUrl &picked, const QUrl &folder, const char *suffix)
{
    if (!picked.isLocalFile() || !folder.isLocalFile()) {
        return picked;
    }

    const QString path = meant(picked.path(QUrl::FullyDecoded), folder.path(QUrl::FullyDecoded));
    return QUrl::fromLocalFile(withSuffix(path, QString::fromLatin1(suffix)));
}
