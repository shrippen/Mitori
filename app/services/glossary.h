#pragma once

#include <QList>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "catalog.h"
#include "searchkey.h"

class Speaker;

// The glossary for QML (clickdummy "Glossar & Suche"): Japanese terms with kanji, German
// translation (Aiki-Budo lexicon of shinkiryu.de) and their pronunciation by speech output.
class Glossary : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // Every word, alphabetical: {word, jp, de, letter, speech}. `letter` heads its section
    // (A … Z), `speech` is what the speaker button says (the kanji, else the reading in kana,
    // else the word).
    Q_PROPERTY(QVariantList words READ words CONSTANT)
    Q_PROPERTY(QString source READ source CONSTANT)
    Q_PROPERTY(Speech speech READ speech NOTIFY speechChanged)

public:
    enum class Speech {
        Unsupported,    // this build or platform has no speech output (AppImage, Flatpak)
        Starting,
        NoVoice,        // no Japanese voice on the device
        Ready,
    };
    Q_ENUM(Speech)

    explicit Glossary(QObject *parent = nullptr);
    // For tests: another catalog and speech engine ("mock").
    Glossary(const Catalog &catalog, const QString &engine, QObject *parent = nullptr);

    QVariantList words() const { return m_words; }
    QString source() const { return m_source; }
    Speech speech() const;

    // Words whose name, kanji, reading or translation contain every word of `query` (see
    // SearchText): "Kiriotoshi", "kiri-otoshi" and 切り落とし find the same, "Hüftwurf" finds
    // Koshi-nage. Rows as in words, alphabetical.
    Q_INVOKABLE QVariantList search(const QString &query) const;
    // Speaks a word's `speech` with the Japanese voice; nothing without one.
    Q_INVOKABLE void say(const QString &speech);

signals:
    void speechChanged();
    // Each call of say(), with or without a voice (the UI test counts them).
    void sayRequested(const QString &speech);

private:
    void load(const Catalog &catalog);

    QVariantList m_words;
    QList<SearchText> m_texts;      // per word, what search() compares
    QString m_source;
    Speaker *m_speaker = nullptr;
};
