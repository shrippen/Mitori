#include "glossary.h"

#include "speaker.h"

namespace {

// The section a word stands in: its first letter without accents (Ô-dachi under O).
QString letter(const QString &word)
{
    const QString key = searchKey(word);
    return key.isEmpty() || !key.front().isLetter() ? QStringLiteral("#") : key.left(1).toUpper();
}

// What the speaker says: the kanji, else the reading in kana, else the word itself.
QString speechText(const QString &word, const QString &jp, const QString &kana)
{
    if (!jp.isEmpty()) {
        return jp;
    }
    return kana.isEmpty() ? word : kana;
}

} // namespace

// MITORI_SPEECH_ENGINE picks a speech engine by name (the UI test: "mock"), as MITORI_VIDEOS the videos.
Glossary::Glossary(QObject *parent)
    : Glossary(Catalog::bundled(), qEnvironmentVariable("MITORI_SPEECH_ENGINE"), parent)
{
}

Glossary::Glossary(const Catalog &catalog, const QString &engine, QObject *parent)
    : QObject(parent)
    , m_speaker(new Speaker(engine, this))
{
    load(catalog);
    connect(m_speaker, &Speaker::stateChanged, this, &Glossary::speechChanged);
}

// From the catalog's fields (German, tools/daten.py) to the rows QML shows.
void Glossary::load(const Catalog &catalog)
{
    m_source = catalog.glossarySource();
    for (const QVariant &w : catalog.glossary()) {
        const QVariantMap entry = w.toMap();
        const QString word = entry.value(QStringLiteral("wort")).toString();
        const QString jp = entry.value(QStringLiteral("jp")).toString();
        const QString de = entry.value(QStringLiteral("de")).toString();
        const QString kana = entry.value(QStringLiteral("kana")).toString();
        m_words << QVariantMap{{QStringLiteral("word"), word},
                               {QStringLiteral("jp"), jp},
                               {QStringLiteral("de"), de},
                               {QStringLiteral("letter"), letter(word)},
                               {QStringLiteral("speech"), speechText(word, jp, kana)}};
        m_texts << SearchText(QStringList{word, jp, kana, de}.join(u' '));
    }
}

Glossary::Speech Glossary::speech() const
{
    switch (m_speaker->state()) {
    case Speaker::State::Unsupported:
        return Speech::Unsupported;
    case Speaker::State::Starting:
        return Speech::Starting;
    case Speaker::State::NoVoice:
        return Speech::NoVoice;
    case Speaker::State::Ready:
        return Speech::Ready;
    }
    return Speech::Unsupported;
}

QVariantList Glossary::search(const QString &query) const
{
    QVariantList result;
    const QStringList words = searchWords(query);
    if (words.isEmpty()) {
        return result;
    }
    for (qsizetype i = 0; i < m_words.size(); ++i) {
        if (m_texts.at(i).matches(words)) {
            result << m_words.at(i);
        }
    }
    return result;
}

void Glossary::say(const QString &speech)
{
    emit sayRequested(speech);
    m_speaker->say(speech);
}
