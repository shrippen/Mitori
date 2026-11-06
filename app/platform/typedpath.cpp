#include "typedpath.h"

#include <QDir>

QUrl TypedPath::resolve(const QUrl &picked, const QUrl &folder)
{
    if (!picked.isLocalFile() || !folder.isLocalFile()) {
        return picked;
    }

    // What was typed: the part after the folder ("/docs//var/x.zip" in /docs: "/var/x.zip"),
    // or the whole path ("file:x.zip").
    const QString path = picked.path(QUrl::FullyDecoded);
    QString base = folder.toLocalFile();
    if (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }
    const QString prefix = base + QLatin1Char('/');
    const QString typed = path.startsWith(prefix) ? path.mid(prefix.size()) : path;

    if (typed.startsWith(QLatin1Char('/')) && typed != path) {
        return QUrl::fromLocalFile(typed);
    }
    if (typed.startsWith(QLatin1String("~/"))) {
        return QUrl::fromLocalFile(QDir::homePath() + typed.mid(1));
    }
    if (!path.startsWith(QLatin1Char('/'))) {
        return QUrl::fromLocalFile(prefix + typed);
    }
    return picked;
}
