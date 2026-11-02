#pragma once

#include <QString>
#include <QList>
#include <QStringList>

#include <algorithm>

inline bool isDash(QChar c)
{
    return c == u'-' || c == u'‐' || c == u'‑' || c == u'‒' || c == u'–' || c == u'—' || c == u'−';
}

// The form search compares: lower case, without accents (Ô-dachi finds o dachi, größer finds
// grosser; kana keep their dakuten), full-width letters folded, dashes, slashes and middle dots
// as spaces, spaces simplified. `keepHyphens` writes dashes as "-" instead.
inline QString searchKey(const QString &text, bool keepHyphens = false)
{
    QString key;
    for (const QChar c : text.normalized(QString::NormalizationForm_KD)) {
        if (c.unicode() >= 0x0300 && c.unicode() <= 0x036F) {
            continue;   // combining accents of Latin letters
        }
        const QChar lower = c.toLower();
        if (lower == u'ß') {
            key += QStringLiteral("ss");
        } else if (isDash(c)) {
            key += keepHyphens ? QChar(u'-') : QChar(u' ');
        } else {
            key += (c == u'/' || c == u'·') ? QChar(u' ') : lower;
        }
    }
    return key.simplified().normalized(QString::NormalizationForm_C);   // kana with dakuten as one
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

// A text to search (a technique's fields, a note): whether every word of a query occurs in it.
// A word matches inside a word, or joined from the start of a word across the following
// hyphens and spaces: across a hyphen it may end anywhere (shihon finds Shiho-nage while
// typing), across a space only where a word ends (nurein finds "nur ein", mitte not "mit
// Tenkan"). A hyphenated word matches joined, or each part from the start of a word
// (ikkyo-irimi as ikkyo irimi; ai-hanmi not in Kaiten-nage). A word with kanji leaves the
// okurigana out on both sides (切り落とし finds 切り落し).
class SearchText
{
public:
    explicit SearchText(const QString &text)
    {
        const QString key = searchKey(text, true);
        m_key = QString(key).replace(u'-', u' ');
        QChar before = u' ';
        for (const QChar c : key) {
            if (c == u' ' || c == u'-') {
                if (!m_joined.isEmpty()) {
                    m_ends << m_joined.size();
                    if (c == u' ') {
                        m_spaces << m_joined.size();
                    }
                }
            } else {
                if (before == u' ' || before == u'-') {
                    m_starts << m_joined.size();
                }
                m_joined += c;
            }
            before = c;
        }
        m_ends << m_joined.size();
    }

    bool matches(const QStringList &words) const
    {
        return std::all_of(words.cbegin(), words.cend(), [this](const QString &w) {
            if (!w.contains(u'-')) {
                return m_key.contains(w) || fromWordStart(w);
            }
            if (fromWordStart(QString(w).remove(u'-'))) {
                return true;
            }
            const QStringList parts = w.split(u'-', Qt::SkipEmptyParts);
            return std::all_of(parts.cbegin(), parts.cend(), [this](const QString &p) { return fromWordStart(p); });
        });
    }

private:
    bool fromWordStart(const QString &w) const
    {
        const QStringView joined(m_joined);
        if (joined.contains(w)) {
            for (const qsizetype at : m_starts) {
                if (!joined.sliced(at).startsWith(w)) {
                    continue;
                }
                const qsizetype end = at + w.size();
                const bool acrossSpace = std::any_of(m_spaces.cbegin(), m_spaces.cend(),
                                                     [&](qsizetype s) { return s > at && s < end; });
                if (!acrossSpace || m_ends.contains(end)) {
                    return true;
                }
            }
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

    QString m_key;                  // as searchKey gives it
    QString m_joined;               // without spaces and hyphens
    QList<qsizetype> m_starts;      // where words begin in m_joined
    QList<qsizetype> m_ends;        // where words end
    QList<qsizetype> m_spaces;      // where a space (not a hyphen) was
    mutable QString m_kanji;
};
