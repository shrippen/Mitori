#pragma once

#include <QUrl>

// What a path typed into Qt's own file dialog means. The app gets that dialog on Plasma outside the
// Flatpak (a QGuiApplication: the KDE theme's dialog needs widgets; seen with that theme, another
// may bring its own). It treats the name field as a name in the current folder and does not expand
// the path field (Ctrl+L):
//
//   in the folder /docs, name field   /var/x.zip  →  file:///docs//var/x.zip   (no such folder)
//                                     ~/x.zip     →  file:///docs/~/x.zip
//                        path field   x.zip       →  file:x.zip                (the process's folder)
//   Qt adds the suffix only when the whole path has no dot: in ~/.cache, y stays y.
namespace TypedPath {

// The file the user meant, with `suffix` ("zip"); Android's content:// as it is.
QUrl resolve(const QUrl &picked, const QUrl &folder, const char *suffix);

}
