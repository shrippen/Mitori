#pragma once

#include <QString>
#include <QVariantList>

// The programme data of all disciplines (mitori-daten.json, built by tools/daten.py from daten/).
// Read by services only, never by QML (agent.md, layers).
class Catalog
{
public:
    static constexpr int kFormat = 1;  // FORMAT in tools/daten.py
    static constexpr auto kResource = ":/daten/mitori-daten.json";

    // The catalog in the app's resources, loaded once and shared by the services.
    static const Catalog &bundled();

    bool load(const QString &path);
    QVariantList disciplines() const { return m_disciplines; }
    // Per video key ("aikido/k5"): datei (path in the video folder), beschnitt {hoehe, y, h, verhaeltnis}.
    QVariantMap videos() const { return m_videos; }
    // The glossary, alphabetical: per word wort, jp (or null), de (or ""), kana (its reading).
    QVariantList glossary() const { return m_glossary; }
    // Where the glossary's translations come from (the lexicon of shinkiryu.de and its date).
    QString glossarySource() const { return m_glossarySource; }
    // The data's release (STAND in mitori-daten), shown under "About Mitori" and on the booklet's title.
    QString dataVersion() const { return m_dataVersion; }
    QString errorString() const { return m_error; }

private:
    bool fail(const QString &error);

    QVariantList m_disciplines;
    QVariantMap m_videos;
    QVariantList m_glossary;
    QString m_glossarySource;
    QString m_dataVersion;
    QString m_error;
};
