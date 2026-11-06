#pragma once

#include <QString>

// The system's share dialog with one file. Android: a chooser (ACTION_SEND) with a content URI from
// the FileProvider of the manifest, which serves only folder() (res/xml/share_paths.xml).
//
// Elsewhere there is none: xdg-desktop-portal has no share interface (OpenURI opens, Email only
// mails), so the desktop offers Save alone.
namespace ShareFile {

bool available();

// Where a file to send goes: the app's cache, in no backup, thrown away if space runs out.
QString folder();

// False when no dialog could be started. `path` lies in folder().
bool send(const QString &path, const QString &subject);

}
