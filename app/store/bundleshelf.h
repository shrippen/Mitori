#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

#include <optional>

// The video bundles on this device: a folder with one subfolder per bundle (the decrypted videos)
// and shelf.json with what belongs to each. A removed bundle keeps its code and key, so it loads
// again without the code (also when the code was blocked since).
//
//   <root>/shelf.json                 {"aikido-kyu": {"code", "key", "version", "videos": {"aikido/k5":
//   <root>/aikido-kyu/aikido-k5.v2.mkv                 {"file", "cropped", "cuts": {"<technique>": [von, bis]}}}}}
//   <root>/manifest.json              the manifest as last loaded (Bundles)
// Tells the users of the shared shelf that bundles came or went (Videos re-reads its files).
class BundleShelfNotifier : public QObject
{
    Q_OBJECT
signals:
    void changed();
};

class BundleShelf
{
public:
    // The shelf in the app's data folder, one for the app (Bundles and Videos).
    static BundleShelf &shared();
    static BundleShelfNotifier &notifier();

    // Where a technique lies in a video's file, in seconds.
    struct Cut {
        double von = 0;
        double bis = 0;
    };
    using Cuts = QHash<QString, QHash<QString, Cut>>;   // video key → technique id → its place

    struct Bundle {
        QString code;               // as the user typed it, normalised
        QByteArray key;             // the content key (from the code)
        int version = 0;            // installed version, 0 when not loaded (or removed)
        QHash<QString, QString> files;      // video key → file name in the bundle's folder
        QStringList cropped;        // the videos whose files are cropped (no black bars)
        Cuts cuts;                  // from the manifest of the loaded version, so they fit its files
    };

    explicit BundleShelf(const QString &root);

    QString root() const { return m_root; }
    // The shelf's bundles by id (unlocked ones, loaded or not).
    QHash<QString, Bundle> bundles() const { return m_bundles; }
    Bundle bundle(const QString &id) const { return m_bundles.value(id); }

    // Keeps the code and key of a bundle (unlocked, not loaded yet); false when it cannot be saved
    // or the id is not valid (as for installed and remove).
    bool unlock(const QString &id, const QString &code, const QByteArray &key);
    // A loaded version: its files (in folder(id)), which are cropped, and the techniques' places.
    bool installed(const QString &id, int version, const QHash<QString, QString> &files, const QStringList &cropped,
                   const Cuts &cuts);
    // The bundle's files go; code and key stay.
    bool remove(const QString &id);

    // A bundle's folder; only for a valid id.
    QString folder(const QString &id) const;
    // Whether an id (from the manifest) can name a folder: "aikido-kyu", not "..", "/x" or "a/b".
    static bool validId(const QString &id);
    // The file of a video from a loaded bundle, or "" (then the video folder of the settings counts).
    QString path(const QString &video) const;
    bool cropped(const QString &video) const;
    // A technique's place in the file path(video) gives, if the bundle names one.
    std::optional<Cut> cut(const QString &video, const QString &id) const;
    // Bytes of all loaded videos.
    qint64 size() const;

    // The default place: "bundles" in the app's data folder.
    static QString defaultRoot();

private:
    void load();
    bool save() const;

    QString m_root;
    QHash<QString, Bundle> m_bundles;
};
