#pragma once

#include <QString>
#include <QList>
#include <QStringList>

#include <algorithm>

// The form search compares: lower case, without accents (Ô-dachi finds o dachi, größer finds
// grosser), full-width letters folded, hyphens, dashes, slashes and middle dots as spaces,
// spaces simplified. `keepHyphens` leaves hyphens and dashes as "-" (for a query's words).
inline QString searchKey(const QString &text, bool keepHyphens = false)
{
    QString key;
    for (const QChar c : text.normalized(QString::NormalizationForm_KD)) {
        if (c.category() == QChar::Mark_NonSpacing) {
            continue;
        }
        if (c == u'ß') {
            key += QStringLiteral("ss");
        } else if (c == u'-' || c == u'–' || c == u'—') {
            key += keepHyphens ? QChar(u'-') : QChar(u' ');
        } else {
            key += (c == u'/' || c == u'·') ? QChar(u' ') : c.toLower();
        }
    }
    return key.simplified();
}

// The words of a query; a word may keep its hyphens (shiho-nage, see SearchText).
inline QStringList searchWords(const QString &query)
{
    QStringList words;
    for (const QString &w : searchKey(query, true).split(u' ', Qt::SkipEmptyParts)) {
        if (!QString(w).remove(u'-').isEmpty()) {
            words << w;
        }
    }
    return words;
}

inline bool isKanji(QChar c)
{
    return c.script() == QChar::Script_Han;
}

inline QString withoutHiragana(QString text)
{
    text.removeIf([](QChar c) { return c.script() == QChar::Script_Hiragana; });
    return text;
}

// Whether every word of the query occurs in the text (as searchKey gives it). From the start of
// a word, a word also matches across spaces and hyphens (shihonage finds Shiho-nage, but rein
// does not find "nur ein"); a hyphenated word matches joined or part by part (ikkyo-irimi as
// ikkyo irimi); a word with kanji leaves the okurigana out on both sides (切り落とし finds 切り落し).
class SearchText
{
public:
    explicit SearchText(const QString &key)
        : m_key(key)
    {
        bool start = true;
        for (const QChar c : key) {
            if (c == u' ') {
                start = true;
                continue;
            }
            if (start) {
                m_starts << m_joined.size();
                start = false;
            }
            m_joined += c;
        }
    }

    bool matches(const QStringList &words) const
    {
        return std::all_of(words.cbegin(), words.cend(), [this](const QString &w) {
            if (!w.contains(u'-')) {
                return matchesWord(w);
            }
            if (matchesWord(QString(w).remove(u'-'))) {
                return true;
            }
            const QStringList parts = w.split(u'-', Qt::SkipEmptyParts);
            return std::all_of(parts.cbegin(), parts.cend(), [this](const QString &p) { return matchesWord(p); });
        });
    }

private:
    bool matchesWord(const QString &w) const
    {
        if (m_key.contains(w)) {
            return true;
        }
        const QStringView joined(m_joined);
        if (joined.contains(w) && std::any_of(m_starts.cbegin(), m_starts.cend(), [&](qsizetype at) { return joined.sliced(at).startsWith(w); })) {
            return true;
        }
        if (!std::any_of(w.cbegin(), w.cend(), isKanji)) {
            return false;
        }
        if (m_kanji.isNull()) {
            m_kanji = withoutHiragana(m_joined);
        }
        const QString bare = withoutHiragana(w);
        return !bare.isEmpty() && m_kanji.contains(bare);
    }

    QString m_key;
    QString m_joined;
    QList<qsizetype> m_starts;
    mutable QString m_kanji;
};
