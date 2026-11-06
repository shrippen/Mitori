#pragma once

#include <QUrl>

// What a path typed into Qt's own file dialog means. That dialog is the one this app gets on the
// desktop outside the Flatpak (a QGuiApplication: no widgets, so no KDE or GTK dialog); it treats
// the name field as a name in the current folder and does not expand the path field (Ctrl+L):
//
//   in the folder /docs, name field   /var/x.zip  →  file:///docs//var/x.zip   (no such folder)
//                                     ~/x.zip     →  file:///docs/~/x.zip
//                        path field   x.zip       →  file:x.zip                (the process's folder)
namespace TypedPath {

// The file the user meant; anything else (a plain pick, Android's content://) as it is.
QUrl resolve(const QUrl &picked, const QUrl &folder);

}
