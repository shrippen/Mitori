#pragma once

#include <QString>
#include <QUrl>

// The photos and voice notes of notes and course entries: one file each in a folder of their own
// (`medien` in the app's data folder), named by a new uid. A note keeps the file's name only.
//
//   <root>/3f2a….jpg    a photo, at most kMaxSide pixels on its long side (JPEG)
//   <root>/91c0….m4a    a voice note (AAC in MP4, as the recorder writes it)
class MediaStore
{
public:
    enum class Kind { None, Photo, Voice };

    explicit MediaStore(const QString &root);
    // The folder in the app's data folder, one for the app.
    static MediaStore &shared();
    static QString defaultRoot();

    QString root() const { return m_root; }
    // A file's kind by its name ("….jpg" a photo, "….m4a" a voice note).
    static Kind kindOf(const QString &name);
    QString path(const QString &name) const;
    QUrl url(const QString &name) const;

    // A photo from a file (picked, on Android also a content:// URL, or the camera's), made
    // smaller and saved as JPEG; its name, or "" when it is no picture.
    QString importPhoto(const QUrl &source);
    // A name for a new voice note; the recorder writes to path(name).
    QString newVoiceName();
    // Removes a file (a deleted note's, a discarded recording); false when there was none.
    bool remove(const QString &name);

    static constexpr int kMaxSide = 2048;
    static constexpr int kQuality = 85;

private:
    QString m_root;
};
