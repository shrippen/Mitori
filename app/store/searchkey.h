#pragma once

#include <QString>
#include <QStringList>

#include <algorithm>

// The form search compares: lower case, without accents (Ô-dachi finds o dachi, größer finds
// grosser), hyphens, slashes and middle dots as spaces, spaces simplified.
inline QString searchKey(const QString &text)
{
    QString key;
    for (const QChar c : text.normalized(QString::NormalizationForm_D)) {
        if (c.category() == QChar::Mark_NonSpacing) {
            continue;
        }
        if (c == u'ß') {
            key += QStringLiteral("ss");
        } else {
            key += (c == u'-' || c == u'/' || c == u'·') ? QChar(u' ') : c.toLower();
        }
    }
    return key.simplified();
}

// The words of a query; a hyphen joins (shiho-nage is one word, as shihonage).
inline QStringList searchWords(const QString &query)
{
    return searchKey(QString(query).remove(u'-')).split(u' ', Qt::SkipEmptyParts);
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

// Whether every word of the query occurs in the text (as searchKey gives it). A word also
// matches across spaces and hyphens (shihonage finds Shiho-nage, kiriotoshi Kiri-otoshi), and a
// word with kanji leaves the okurigana out on both sides (切り落とし finds 切り落し).
class SearchText
{
public:
    explicit SearchText(const QString &key)
        : m_key(key)
        , m_joined(QString(key).remove(u' '))
    {
    }

    bool matches(const QStringList &words) const
    {
        return std::all_of(words.cbegin(), words.cend(), [this](const QString &w) {
            if (m_key.contains(w) || m_joined.contains(w)) {
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
        });
    }

private:
    QString m_key;
    QString m_joined;
    mutable QString m_kanji;
};
