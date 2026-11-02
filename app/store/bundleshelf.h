#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

// The video bundles on this device: a folder with one subfolder per bundle (the decrypted videos)
// and shelf.json with what belongs to each. A removed bundle keeps its code and key, so it loads
// again without the code (also when the code was blocked since).
//
//   <root>/shelf.json                 {"aikido-kyu": {"code", "key", "version", "videos": {"aikido/k5":
//   <root>/aikido-kyu/aikido-k5.mkv                    {"file", "cropped"}}}}
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

    struct Bundle {
        QString code;               // as the user typed it, normalised
        QByteArray key;             // the content key (from the code)
        int version = 0;            // installed version, 0 when not loaded (or removed)
        QHash<QString, QString> files;      // video key → file name in the bundle's folder
        QStringList cropped;        // the videos whose files are cropped (no black bars)
    };

    explicit BundleShelf(const QString &root);

    QString root() const { return m_root; }
    // The shelf's bundles by id (unlocked ones, loaded or not).
    QHash<QString, Bundle> bundles() const { return m_bundles; }
    Bundle bundle(const QString &id) const { return m_bundles.value(id); }

    // Keeps the code and key of a bundle (unlocked, not loaded yet); false when it cannot be saved.
    bool unlock(const QString &id, const QString &code, const QByteArray &key);
    // A loaded version: its files (in folder(id)) and which are cropped.
    bool installed(const QString &id, int version, const QHash<QString, QString> &files, const QStringList &cropped);
    // The bundle's files go; code and key stay.
    bool remove(const QString &id);

    QString folder(const QString &id) const;
    // The file of a video from a loaded bundle, or "" (then the video folder of the settings counts).
    QString path(const QString &video) const;
    bool cropped(const QString &video) const;
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
